/*
 * proxy.c
 */

#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h>

#include <pthread.h>

#include <string.h>

#include "hash.h" 
#include "log.h" 
#include "proxy.h" 
#include "proxy_ext.h" 
#include "lowlevel.h"
#include "http.h"

extern hashtable *cfg;

pthread_mutex_t proxy_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t proxy_validate_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t proxy_vthreads_mutex = PTHREAD_MUTEX_INITIALIZER;

static proxy_db proxies;
static int proxy_timeout, proxy_lifetime, vthreads_counter;

/*
 * proxy_get_iotimeout, return timeout for proxy i/o
 */

int proxy_get_iotimeout(void) {
	int retval;

	/* CRITICAL SECTION */
	pthread_mutex_lock(&proxy_mutex);
	retval = proxy_timeout;	
	pthread_mutex_unlock(&proxy_mutex);
	/* CRITICAL SECTION */

	return retval;
}

/*
 * proxy_build_limits, create/update proxy limits variables
 */

void proxy_build_limits(void) {

	/* CRITICAL SECTION */
	pthread_mutex_lock(&proxy_mutex);
	proxy_timeout = atoi(hashtable_lookup(cfg, "proxy-timeout"));
	proxy_lifetime = atoi(hashtable_lookup(cfg, "proxy-deadhits"));
	pthread_mutex_unlock(&proxy_mutex);
	/* CRITICAL SECTION */

	return ;
}

/*
 * proxy_parse, parse proxy database
 */

int proxy_parse(void) {
	FILE *fp;
	char buf[1024], *proxy_addr, *proxy_port, *proxyfile, *retval;
	proxy_entry cur_proxy;
	int lines;

	proxyfile = hashtable_lookup(cfg, "proxy-file");

	if (!proxyfile) {
		return 0;
	}

	fp = fopen(proxyfile, "r");

	if (!fp) {
		perror(proxyfile);
		return 0;
	}

	proxies.db = calloc(sizeof(proxy_entry*), _MAX_PROXIES);

	if (!proxies.db) {
		perror("calloc");
		return 0;
	}
	
	lines = 0;

	while (!feof(fp)) {

		if (fgets(buf, sizeof(buf), fp) == NULL) {
			break;
		}

		lines++;

		if (strlen(buf) < 2 || buf[0] == '#') {
			continue;
		}

		buf[strlen(buf)-1] = '\0';

		if (!strstr(buf, ":")) {
			lprintf("[proxy/error] invalid proxy entry at line #%d, no delimiter!\n", lines);
			continue;
		}
		
                proxy_port = strstr(buf, ":") + 1;
                proxy_addr = strtok(buf, ":");

                if (!proxy_addr) {
			lprintf("[proxy/error] invalid proxy entry at line #%d\n", lines);
                        continue;
                }

		cur_proxy.addr = strdup(proxy_addr);
		cur_proxy.port = atoi(proxy_port);
		cur_proxy.strikes = 0;
		cur_proxy.state = UNKNOWN;
		cur_proxy.jobs = 0;

		proxies.db[proxies.stats.index] = calloc(1, sizeof(proxy_entry));

		if (!proxies.db[proxies.stats.index]) {
			perror("calloc");
			continue;
		}

		memcpy(proxies.db[proxies.stats.index], &cur_proxy, sizeof(proxy_entry));

		proxies.stats.index++;
	}

	fclose(fp);

	lprintf("[proxy/notice] parsed %s (%d lines, %d proxies)\n", proxyfile, lines, proxies.stats.index);

	proxies.stats.unknowns = proxies.stats.index;

	proxy_build_limits();

	if (proxies.stats.index < 1) {
		lprintf("[proxy/notice] no proxies were found in %s, ignored!\n", proxyfile);
		return 1;
	}

	retval = hashtable_lookup(cfg, "proxy-validuponload");

	if (retval && !strcmp(retval, "on")) {
		proxy_validateall();
	}
	
	return 1;
}

/*
 * proxy_validate, validate proxy
 * * id, given proxy id
 */

int proxy_validate(int id) {
	proxy_entry *cur;
	int proxy_fd = -1;

	/* CRITICAL SECTION */
	pthread_mutex_lock(&proxy_validate_mutex);

	cur = proxies.db[id];

	proxy_fd = net_opensocket(cur->addr, cur->port, proxy_timeout);

	if (proxy_fd < 0) {

		cur->strikes++;

		if (cur->strikes == proxy_lifetime || cur->state == UNKNOWN) {

			switch (cur->state) {
									
				case BROKEN:
					proxies.stats.broken--;
					break;

				case WORKING:
					proxies.stats.valids--;
					break;

				case UNKNOWN:
					proxies.stats.unknowns--;
					break;
			}

			proxies.stats.deads++;
			cur->state = DEAD;
		}

	} else {

		switch (cur->state) {

			case DEAD:
				proxies.stats.deads--;
				proxies.stats.valids++;
				cur->state = WORKING;

			case WORKING:

				if (cur->strikes) {
					cur->strikes--;
				}

				break;

			case UNKNOWN:
				proxies.stats.unknowns--;
				proxies.stats.valids++;
				cur->state = WORKING;
				break;

		}
	}

	pthread_mutex_unlock(&proxy_validate_mutex);
	/* CRITICAL SECTION */

	return proxy_fd;
}

/*
 * proxy_t_validate, thread-safe wrap for proxy_validate 
 * * id, given proxy id
 */

int proxy_t_validate(int id) {
	int rval;

	/* CRITICAL SECTION */
	pthread_mutex_lock(&proxy_mutex);
	rval = proxy_validate(id);
	pthread_mutex_unlock(&proxy_mutex);
	/* CRITICAL SECTION */

	return rval;	
}

/*
 * proxy_getrandproxy, get random proxy for action
 */

int proxy_getrandproxy(void) {
	int proxy_fd = -1, blackjack;
	proxy_entry *cur;

	/* CRITICAL SECTION */
	pthread_mutex_lock(&proxy_mutex);

	while ((proxies.stats.valids+proxies.stats.unknowns) > 0) {

		srand(time(NULL));

		blackjack = rand() % proxies.stats.index;

		cur = proxies.db[blackjack];

		if (!cur || cur->state == DEAD || cur->state == BROKEN) {
			continue;
		}

		proxy_fd = proxy_validate(blackjack);
		
		if (proxy_fd > 0) {
			cur->jobs++;
			break;
		}
	}

	pthread_mutex_unlock(&proxy_mutex);
	/* CRITICAL SECTION */

	return proxy_fd; 
}

/*
 * proxy_get_stats, return current proxies database snapshot
 */

proxy_db_stats *proxy_get_stats(void) {

	proxy_db_stats *cur;

	/* CRITICAL SECTION */
	pthread_mutex_lock(&proxy_mutex);
	
	cur = calloc(1, sizeof(proxy_db_stats));

	if (!cur) {
		lerror("calloc");
		pthread_mutex_unlock(&proxy_mutex);
		return NULL;
	}

	memcpy(cur, &proxies.stats, sizeof(proxy_db_stats));

	pthread_mutex_unlock(&proxy_mutex);
	/* CRITICAL SECTION */

	return cur;
}

/*
 * proxy_querydb, query proxy database for given entry
 * * id, given entry id
 */

proxy_entry *proxy_querydb(int id) {
	proxy_entry *res;

	/* CRITICAL SECTION */
	pthread_mutex_lock(&proxy_mutex);
	
	if (proxies.stats.index < id || !proxies.db[id]) {
		pthread_mutex_unlock(&proxy_mutex);
		return NULL;
	}

	res = calloc(1, sizeof(proxy_entry));

	if (!res) {
		lerror("calloc");
		pthread_mutex_unlock(&proxy_mutex);
		return NULL;
	}
	
	memcpy(res, proxies.db[id], sizeof(proxy_entry));
	pthread_mutex_unlock(&proxy_mutex);
	/* CRITICAL SECTION */

	return res;
}

/*
 * proxy_rebuild_db, rebuild proxy database and remove dead proxies & patch holes
 */

void proxy_rebuild_db(void) {
	int idx, holes;

	/* CRITICAL SECTION */
	pthread_mutex_lock(&proxy_mutex);
	
	holes = proxies.stats.deads = proxies.stats.holes = proxies.stats.broken = proxies.stats.valids = proxies.stats.unknowns = 0;

	for (idx = 0; idx < proxies.stats.index - holes; idx++) {

		if (!proxies.db[idx]) {
			holes++;
			proxies.db[idx] = proxies.db[idx+1];
			idx--;
			continue;
		}

		if (proxies.db[idx]->state == UNKNOWN) {
			proxies.stats.unknowns++;
			continue;
		}

		if (proxies.db[idx]->state == WORKING) {
			proxies.stats.valids++;
			continue;
		}

		if (proxies.db[idx]->state == DEAD || proxies.db[idx]->state == BROKEN) {

			if (proxies.db[idx]->addr) {
				free(proxies.db[idx]->addr);
				proxies.db[idx]->addr = NULL;
			}

			if (proxies.db[idx]) { 
				free(proxies.db[idx]);
				proxies.db[idx] = NULL;
			}

			continue;
		}
	}

	proxies.stats.index = idx;

	pthread_mutex_unlock(&proxy_mutex);
	/* CRITICAL SECTION */

	return ;
}

/*
 * proxy_mod, modify a proxy entry in the database
 * * idx, given proxy entry index
 * * addr, given proxy address
 * * port, given proxy port
 */

int proxy_mod(int idx, char *addr, int port) {
	char *p_addr;

	/* CRITICAL SECTION */
	pthread_mutex_lock(&proxy_mutex);

	if (proxies.stats.index < idx) {
		pthread_mutex_unlock(&proxy_mutex);
		return -1;
	}

	if (strcmp(proxies.db[idx]->addr, addr)) {

		p_addr = strdup(addr);

		if (!p_addr) {
			lerror("strdup");
			pthread_mutex_unlock(&proxy_mutex);
			return -1;
		}

		free(proxies.db[idx]->addr);
		proxies.db[idx]->addr = p_addr;
	}

	if (proxies.db[idx]->port != port) {
		proxies.db[idx]->port = port;
	}

	pthread_mutex_unlock(&proxy_mutex);
	/* CRITICAL SECTION */

	return 1;	
}

/*
 * proxy_add, add proxy entry to database
 * * addr, given proxy address
 * * port, given proxy port
 */

int proxy_add(char *addr, int port) {

	/* CRITICAL SECTION */
	pthread_mutex_lock(&proxy_mutex);

	if (proxies.stats.index == _MAX_PROXIES) {
		pthread_mutex_unlock(&proxy_mutex);
		return -1;
	}

	proxies.db[proxies.stats.index] = calloc(1, sizeof(proxy_entry));

	if (!proxies.db[proxies.stats.index]) {
		lerror("calloc");
		pthread_mutex_unlock(&proxy_mutex);
		return -1;
	}

	proxies.db[proxies.stats.index]->addr = strdup(addr);

	if (!proxies.db[proxies.stats.index]->addr) {
		free(proxies.db[proxies.stats.index]);
		proxies.db[proxies.stats.index] = NULL;
		lerror("calloc");
		return -1;
	}

	proxies.db[proxies.stats.index]->port = port;
	proxies.db[proxies.stats.index]->state = UNKNOWN;

	proxies.stats.index++;
	proxies.stats.unknowns++;

	pthread_mutex_unlock(&proxy_mutex);
	/* CRITICAL SECTION */

	return 1;
}

/*
 * proxy_delete, delete proxy entry from database
 * * idx, given proxy entry index
 */

int proxy_delete(int idx) {

	/* CRITICAL SECTION */
	pthread_mutex_lock(&proxy_mutex);

	if (proxies.stats.index < idx || !proxies.db[idx]) {
		pthread_mutex_unlock(&proxy_mutex);
		return -1;
	}	

	switch (proxies.db[idx]->state) {

		case BROKEN:
			proxies.stats.broken--;
			break;

		case UNKNOWN:
			proxies.stats.unknowns--;
			break;

		case DEAD:
			proxies.stats.deads--;
			break;

		case WORKING:
			proxies.stats.valids--;
			break;
	}

	if (proxies.db[idx]->addr) {
		free(proxies.db[idx]->addr);
		proxies.db[idx]->addr = NULL;
	}

	free(proxies.db[idx]);
	proxies.db[idx] = NULL;

	if (idx == (proxies.stats.index - 1)) {
		proxies.stats.index--;
	} else {
		proxies.stats.holes++;
	}

	pthread_mutex_unlock(&proxy_mutex);	
	/* CRITICAL SECTION */

	return 1;
}

/*
 * proxy_getrank, find rank for given proxy based on strikes and jobs
 * * proxy, given proxy entry
 */

char *proxy_getrank(proxy_entry *proxy) {

	char *retval = NULL;

	switch (proxy->state) {

		case BROKEN:

			retval = "broken";
			break;

		case DEAD:

			retval = "dead";
			break;

		case UNKNOWN:

			retval = "unknown";
			break;

		case WORKING:

			if (!proxy->strikes) {

				if (!proxy->jobs) {
				
					retval = "fresh";
					break;

				} else {

					retval = "excellent";
					break;
				}

			} else {

				if (!proxy->jobs) {

					retval = "dead";
					break;
				}

				if ( (proxy->strikes / proxy->jobs * 100) < 50) {

					retval = "good";

				} else {

					retval = "poor";
				}
			}

			break;
	}
	
	return retval;
}

/*
 * proxy_dump, dump current proxies database into file
 * * fp, given file pointer
 */

int proxy_dump(FILE *fp) {
	int i;

	/* CRITICAL SECTION */
	pthread_mutex_lock(&proxy_mutex);

	if (!proxies.stats.valids) {
		return 0;
	}

	fprintf(fp, "# Ghost Proxies Snapshot (%d valid proxies)\n", proxies.stats.valids);

	for (i = 0; i < proxies.stats.index; i++) {

		if (!proxies.db[i] || !proxies.db[i]->addr || proxies.db[i]->state != WORKING) {
			continue;
		}

		fprintf(fp, "%s:%d\n", proxies.db[i]->addr, proxies.db[i]->port);
	}

	fprintf(fp, "# EOF\n");

	pthread_mutex_unlock(&proxy_mutex);
	/* CRITICAL SECTION */

	return 1;
}

/*
 * proxy_http_validate, tests if proxy accepts HTTP packets and response correctly
 * * idx, given proxy entry index
 * * traffic, given buffer for traffic debug
 */

int proxy_http_validate(int idx, char **traffic) {
	char client_buffer[512], proxy_buffer[1024];
	int eoh, fd;

	snprintf(client_buffer, sizeof(client_buffer), "GET http://www.cnn.com HTTP/1.1\r\nHost: www.cnn.com\r\n\r\n");

	fd = proxy_validate(idx);

	if (fd < 0) {
		return -1;
	}

	/* CRITICAL SECTION */
	pthread_mutex_lock(&proxy_validate_mutex);

	if (proxies.db[idx]->state == WORKING) {
		proxies.db[idx]->state = BROKEN;
		proxies.stats.broken++;
		proxies.stats.valids--;
	}

	pthread_mutex_unlock(&proxy_validate_mutex);
	/* CRITICAL SECTION */

	if ( async_write(fd, client_buffer, strlen(client_buffer), proxy_timeout) < 0 ||
			async_read(fd, proxy_buffer, sizeof(proxy_buffer), proxy_timeout) < 0) 
	{
		close(fd);
		return -1;
	}

	close(fd);

	eoh = http_gethdrsize(proxy_buffer);

	if (eoh > 5) {
		proxy_buffer[eoh-4] = '\0';
	}

	if (traffic) {

		*traffic = calloc(1, strlen(client_buffer) + strlen(proxy_buffer) + 1);

		if (!traffic) {

			lerror("calloc");

		} else {

			strcat(*traffic, client_buffer);
			strcat(*traffic, proxy_buffer);
		}
	}

	if (strstr(proxy_buffer, "200 OK") || strstr(proxy_buffer, "302 Found")) {

		/* CRITICAL SECTION */
		pthread_mutex_lock(&proxy_validate_mutex);
		proxies.db[idx]->state = WORKING;
		proxies.stats.broken--;
		proxies.stats.valids++;
		pthread_mutex_unlock(&proxy_validate_mutex);
		/* CRITICAL SECTION */

		return 1;
	}

	return 0;
}

/*
 * proxy_t_validate_http, thread-safe wrap for proxy_http_validate
 * * id, given proxy id
 * * buf, given buffer
 */

int proxy_t_validate_http(int id, char **buf) {
	int rval;

	/* CRITICAL SECTION */
	pthread_mutex_lock(&proxy_mutex);
	rval = proxy_http_validate(id, buf);
	pthread_mutex_unlock(&proxy_mutex);
	/* CRITICAL SECTION */

	return rval;	
}

/*
 * proxy_validation_thread, validation thread entry point
 * * arg, passed proxy entry index
 */

void *proxy_validation_thread(void *arg) {
	int idx;

	pthread_detach(pthread_self());

	/* CRITICAL SECTION */
	pthread_mutex_lock(&proxy_vthreads_mutex);
	vthreads_counter++;
	pthread_mutex_unlock(&proxy_vthreads_mutex);
	/* CRITICAL SECTION */

	idx = (int) arg;

	if (proxies.stats.index < idx || !proxies.db[idx]) {

		/* CRITICAL SECTION */
		pthread_mutex_lock(&proxy_vthreads_mutex);
		vthreads_counter--;
		pthread_mutex_unlock(&proxy_vthreads_mutex);
		/* CRITICAL SECTION */

		pthread_exit(NULL);
	}			

	proxies.db[idx]->state = UNKNOWN;

	proxy_http_validate(idx, NULL);

	/* CRITICAL SECTION */
	pthread_mutex_lock(&proxy_vthreads_mutex);
	vthreads_counter--;
	pthread_mutex_unlock(&proxy_vthreads_mutex);
	/* CRITICAL SECTION */

	pthread_exit(NULL);
}

/*
 * proxy_get_totvalidation_threads, get number of current running validation threads
 */

int proxy_get_totvalidation_threads(void) {
	int retval;

        /* CRITICAL SECTION */
        pthread_mutex_lock(&proxy_vthreads_mutex);
        retval = vthreads_counter;
        pthread_mutex_unlock(&proxy_vthreads_mutex);
        /* CRITICAL SECTION */

	return retval;
}

/*
 * proxy_validateall, proxies validation thread
 */

int proxy_validateall(void) {
	int i, cur_threads, max_threads = 1, tot_proxies;
	char *retval;
	pthread_t v_thread;

	/* CRITICAL SECTION */
	pthread_mutex_lock(&proxy_mutex);

	proxies.stats.unknowns += proxies.stats.valids + proxies.stats.deads + proxies.stats.broken;
	proxies.stats.valids = proxies.stats.deads = proxies.stats.broken = 0;

	retval = hashtable_lookup(cfg, "proxy-validate-threads");

	if (retval) {
		max_threads = atoi(retval);
	}
	
	lprintf("[proxy/notice] validating %d proxies using %d threads ... hold on!\n", proxies.stats.index, max_threads);

	i = cur_threads = 0;

	tot_proxies = proxies.stats.index;

	while (i < tot_proxies) {

		cur_threads = proxy_get_totvalidation_threads();

		for (; cur_threads < max_threads; cur_threads++) {

			if (pthread_create(&v_thread, NULL, proxy_validation_thread, (int *)i) < 0) {
				lerror("pthread_create");
				cur_threads--;
				sleep(1);
				continue;
			}

			i++;

			sleep(1);
		}

		sleep(1);
	}

	do {
		cur_threads = proxy_get_totvalidation_threads();
		sleep(1);

	} while (cur_threads);

	lprintf("[proxy/notice] -----------------------------------\n");
	lprintf("[proxy/notice] validation status:\n");

	if (proxies.stats.deads) {
		lprintf("[proxy/notice]\t%d - dead proxies\n", proxies.stats.deads);
	} else {
		lprintf("[proxy/notice]\t* no dead proxies!\n");
	}

	if (proxies.stats.broken) {
		lprintf("[proxy/notice]\t%d - broken proxies\n", proxies.stats.broken);
	} else {
		lprintf("[proxy/notice]\t* no broken proxies!\n");
	}

	if (proxies.stats.valids) {
		lprintf("[proxy/notice]\t%d - valid proxies\n", proxies.stats.valids);
	} else {
		lprintf("[proxy/notice]\t* no valid proxies!\n");
	}

	lprintf("[proxy/notice] -----------------------------------\n");

	pthread_mutex_unlock(&proxy_mutex);
	/* CRITICAL SECTION */
	
	return 1;
}
