/*
 * daemon.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <netdb.h>

#include <pthread.h>

#include <fcntl.h>
#include <errno.h>

#include "hash.h"
#include "log.h"
#include "httpd.h"
#include "http.h"
#include "http_ext.h"
#include "proxy.h"
#include "lowlevel.h"
#include "daemon.h"
#include "conf.h"

extern hashtable *cfg;

/*
 * daemon_chunk_judge, whatever chunk should be modify before passed back
 * * args, passed from callback function
 */

__inline__ void *daemon_chunk_judge(char **buf, int len, int flags) {
	if (flags) {
		return http_modify_chunk(buf, len, flags);
	}
	return NULL;
}

/*
 * daemon_to_proxy, callback event once client has data for proxy
 * * buf, given buffer
 * * len, given buffer length
 */

void *daemon_to_proxy(char **buf, int len) {
	return daemon_chunk_judge(buf, len, cfg_get_proxyopts());
}

/*
 * daemon_to_client, callback event once proxy has data for client
 * * buf, given buffer
 * * len, given buffer length
 */

void *daemon_to_client(char **buf, int len) {
	return daemon_chunk_judge(buf, len, cfg_get_clientopts());
}

/*
 * daemon_root_threadproc, thread entry point
 * * args, given fd
 */

void *daemon_root_threadproc(void *args) {
	int fd, proxy_fd, rval;
	char virgin_chunk[2048], *retval, *url, *v_url;

	signal(SIGPIPE, SIG_IGN);

	pthread_detach(pthread_self());

	fd = (int) args;

	retval = hashtable_lookup(cfg, "http-analyzer");

	if (retval && !strcmp(retval, "on")) {

		rval = async_read(fd, virgin_chunk, sizeof(virgin_chunk), proxy_get_iotimeout());

		if (rval < 0){ 
			close(fd);
			pthread_exit(NULL);
		}

		url = http_get_url(virgin_chunk);

		if (!url) {
			lprintf("[http/error] bad/corrupted http chunk:\n\n%s\n", virgin_chunk);
			close(fd);
			pthread_exit(NULL);
		}			

		retval = hashtable_lookup(cfg, "http-admin-mode");

		if (retval && !strcmp(retval, "on")) {

			v_url = hashtable_lookup(cfg, "http-admin-url");

			if (!strcmp(url, v_url) || strstr(url, v_url)) {
				httpd_handler(fd, rindex(url, '/'), virgin_chunk);
				free(url);
				close(fd);
				pthread_exit(NULL);
			}	
		}
		
		proxy_fd = proxy_getrandproxy();

		if (proxy_fd < 0) {
			httpd_www_noproxies_msg(fd);
			free(url);
			close(fd);
			pthread_exit(NULL);
		}

		lprintf("[proxy/notice] %s <--> %s\n", url, net_getpeeraddr(proxy_fd));

		free(url);

		async_duplex_bridge(fd, proxy_fd, daemon_to_client, daemon_to_proxy, virgin_chunk, rval);

	} else {

		proxy_fd = proxy_getrandproxy();

		if (proxy_fd < 0) {
			httpd_www_noproxies_msg(fd);
			close(fd);
			pthread_exit(NULL);
		}

		lprintf("[proxy/notice] Surfing through %s\n", net_getpeeraddr(proxy_fd));

		async_duplex_bridge(fd, proxy_fd, NULL, NULL, NULL, -1);
	}

        close(fd);
        close(proxy_fd);
	
	pthread_exit(NULL);
}

/*
 * daemon_writepid, write .pid file for crontabs and watch dogs
 */

int daemon_writepid(void) {
	char *retval;
	FILE *pid_fd;

	retval = hashtable_lookup(cfg, "daemon-pidfile");

	if (!retval) {
		return 1;
	}

	remove(retval);

	pid_fd = fopen(retval, "a");

	if (!pid_fd) {
		lerror(retval);
		return 0;
	}

	fprintf(pid_fd, "%d\n", getpid());
	
	fclose(pid_fd);

	return 1;
}

/*
 * daemon_start, ghost daemon firecracker
 */

void daemon_start(void) {

	int srv_fd, new_fd;
	pthread_t thread;

	signal(SIGPIPE, SIG_IGN);

	srv_fd = net_mastersocket(hashtable_lookup(cfg, "net-bindip"), hashtable_lookup(cfg, "net-bindport"));

	if (!daemon_writepid()) {
		return ;
	}

	while (srv_fd > 0) {

		new_fd = net_acceptsocket(srv_fd);
		
		if (new_fd < 0) {
			lerror("accept");
			continue;
		}

		if (net_setononblock(new_fd) < 0) {
			lerror("fcntl");
			close(new_fd);
			continue;
		}

		if (pthread_create(&thread, NULL, daemon_root_threadproc, (int *)new_fd) < 0) {
			lerror("pthread_create");
			close(new_fd);
		}
	}

	return ;
}
