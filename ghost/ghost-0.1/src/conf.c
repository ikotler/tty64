/*
 * conf.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <pthread.h>
#include <ctype.h>

#include "log.h"
#include "hash.h"
#include "conf.h"
#include "conf_ext.h"
#include "http_ext.h"

pthread_mutex_t conf_mutex = PTHREAD_MUTEX_INITIALIZER;

static int cfg_client_opts, cfg_proxy_opts;

int cfg_typechk_string(char *, int);
int cfg_typechk_boolean(char *, int);
int cfg_typechk_numeric(char *, int);
int cfg_typechk_url(char *, int);

extern hashtable *cfg;

/*
 * configuration scheme
 */

cfg_validate_entry cfg_truthtable[] = {
	{ "net-bindport"           , IS_NUMERIC , MANDATORY , NULL 	        , STATIC  } ,
	{ "net-bindip"             , IS_STRING  , OPTINIAL  , NULL 	        , STATIC  } ,
	{ "http-analyzer"          , IS_BOOLEAN , OPTINIAL  , NULL 	        , DYANMIC } ,
	{ "http-filter-setcookies" , IS_BOOLEAN , OPTINIAL  , "http-analyzer"   , DYANMIC } ,
	{ "http-filter-getcookies" , IS_BOOLEAN , OPTINIAL  , "http-analyzer"   , DYANMIC } ,
	{ "http-frenzymode"        , IS_BOOLEAN , OPTINIAL  , "http-analyzer"   , DYANMIC } ,
	{ "http-admin-mode"        , IS_BOOLEAN , OPTINIAL  , "http-analyzer"   , DYANMIC } ,
	{ "http-admin-url"	   , IS_URL     , OPTINIAL  , "http-admin-mode" , DYANMIC } ,
	{ "daemon-logfile"         , IS_STRING  , MANDATORY , NULL 	        , STATIC  } ,
	{ "daemon-pidfile"	   , IS_STRING  , OPTINIAL  , NULL 	        , STATIC  } ,
	{ "proxy-file"	           , IS_STRING  , MANDATORY , NULL 	        , STATIC  } ,
	{ "proxy-deadhits"	   , IS_NUMERIC , MANDATORY , NULL 	        , DYANMIC } ,
	{ "proxy-timeout"	   , IS_NUMERIC , MANDATORY , NULL 	        , DYANMIC } ,
	{ "proxy-validuponload"    , IS_BOOLEAN , OPTINIAL  , NULL		, DYANMIC } ,
	{ "proxy-validate-threads" , IS_NUMERIC , OPTINIAL  , NULL		, DYANMIC } ,
	{ NULL			   , -1	        , -1        , NULL 	        , -1      }
};

/*
 * types validation callbacks
 */

cfg_validate_cb cfg_typechecker[] = {
	{ IS_STRING   , "STRING"         , cfg_typechk_string  } ,
	{ IS_NUMERIC  , "NUMERIC"        , cfg_typechk_numeric } ,
	{ IS_BOOLEAN  , "BOOLEAN"        , cfg_typechk_boolean } ,
	{ IS_URL      ,	"HTTP URL"       , cfg_typechk_url     }
};

/*
 * http* opts -> bitmask
 */

cfg_bitmask_opt cfg_bitwise_opts[] = {
	{ "http-filter-setcookies" , HTTP_KILL_COOKIES , HTTP_SERVER_SIDE },
	{ "http-filter-getcookies" , HTTP_KILL_COOKIES , HTTP_CLIENT_SIDE },
	{ "http-frenzymode"        , HTTP_FRENZY       , HTTP_SERVER_SIDE },
	{ NULL			   , -1		       , -1               }
};

/*
 * _threadsafe_get, thread safe/sync variable access
 * * var, given variable
 */

__inline__ void * _threadsafe_conf_get(void *var) {
        void *rval;

        /* CRITICAL SECTION */
        pthread_mutex_lock(&conf_mutex);
        rval = var;
        pthread_mutex_unlock(&conf_mutex);
        /* CRITICAL SECTION */

        return rval;
}

/*
 * cfg_get_proxyopts, get proxy bitmask opts
 */

int cfg_get_proxyopts(void) {
	return (int) _threadsafe_conf_get((int *)cfg_proxy_opts);
}

/*
 * cfg_get_clientopts, get client bitmask opts
 */

int cfg_get_clientopts(void) {
	return (int) _threadsafe_conf_get((int *)cfg_client_opts);
}

/*
 * cfg_parse, parse config file
 * * cfgfile, given config file
 */

hashtable *cfg_parse(char *cfgfile) {
	FILE *fp;
	char *val, *tag, buf[1024];
	hashtable *c_cfg = NULL;
	int lines, cfg_tot_opts;

	fp = fopen(cfgfile, "r");

	if (!fp) {
		perror(cfgfile);
		return NULL;
	}

	c_cfg = hashtable_create(_MAX_CONF_OPTS);
	
	if (!c_cfg) {
		perror("calloc");
		return NULL;
	}

	lines = cfg_tot_opts = 0;

	if (hashtable_insert(c_cfg, "config-file", cfgfile) < 0) {
		perror("hashtable_insert");
		return NULL;
	}

	while (!feof(fp)) {

		if (fgets(buf, sizeof(buf), fp) == NULL) {
			break;
		}

		lines++;

		if (strlen(buf) < 2 || buf[0] == '#') {
			continue;
		}

		buf[strlen(buf)-1] = '\0';

		val = strstr(buf, "= ");

		if (!val) {
			fprintf(stderr, "*** %s : syntax error, no delimiter at line #%d\n", cfgfile, lines);
			return NULL;
		}

		val += 2;

                tag = strtok(buf, " =");
		
                if (!tag) {
			fprintf(stderr, "*** %s : syntax error, missing tag for \"%s\", at line #%d\n", cfgfile, val, lines);
			return NULL;
                }

		if (hashtable_insert(c_cfg, tag, val) < 0) {
			perror("hashtable_insert_in_conf");
			return NULL;
		}

		cfg_tot_opts++;
	}

	fclose(fp);
	
	if (!cfg_tot_opts) {

		fprintf(stderr, "*** %s : corrupted/malformed config file, no tags were parsed!\n", cfgfile);
		return NULL;
	}

	if (_MAX_CONF_OPTS < cfg_tot_opts) {

		fprintf(stderr, "*** %s : corrupted/malformed config file, too many tags parsed and only %d are supported!\n", 
			cfgfile, _MAX_CONF_OPTS);

		return NULL;
	}

	cfg_client_opts = cfg_proxy_opts = HTTP_NOP;

	return c_cfg;
}

/*
 * build_cfg_bitmask_opts, convert http* options into bitmask
 * * c_cfg, current configuration hashtable
 */

void build_cfg_bitmask_opts(hashtable *c_cfg) {
	int i;
	char *retval;

	/* CRITICAL SECTION */
	pthread_mutex_lock(&conf_mutex);

	for (i = 0; cfg_bitwise_opts[i].opt_name != NULL; i++) {

		retval = hashtable_lookup(c_cfg, cfg_bitwise_opts[i].opt_name);

		if (retval && !strcmp(retval, "on")) {

			if (cfg_bitwise_opts[i].side_effected == HTTP_CLIENT_SIDE) {
				cfg_client_opts |= cfg_bitwise_opts[i].bit_value;
			} else {
				cfg_proxy_opts |= cfg_bitwise_opts[i].bit_value;
			}
		}
	}

	pthread_mutex_unlock(&conf_mutex);
	/* CRITICAL SECTION */

	return ;
}

/*
 * cfg_validate, validate given value for given entry
 * * idx, given index
 * * val, given value
 */

int cfg_validate(int idx, char *val) {
	cfg_validate_cb type_check;

	type_check = cfg_typechecker[cfg_truthtable[idx].tag_type];
	
	if (!type_check.validate(val, strlen(val))) {

		if (type_check.type_name) {

			lprintf("*** %s : given value (%s) isn't matchs tag type (%s)\n",
				cfg_truthtable[idx].tag, val, type_check.type_name);

		} else {

			lerror(val);
		}
			
		return 0;
	}

	return 1;
}

/*
 * cfg_validate_all, validate current configuration
 * * c_cfg, given configuration
 * * mode, given validation mode
 */

int cfg_validate_all(hashtable *c_cfg, int mode) {
	int i;
	char *retval, *prefix;

	switch (mode) {

		case ERROR_MODE:
			prefix = "***";
			break;

		case GLITCH_MODE:
			prefix = "[ghost/notice]";
			break;

		default:
			prefix = "+++";
			break;

	}

	for (i = 0; cfg_truthtable[i].tag != NULL; i++) {

		retval = hashtable_lookup(c_cfg, cfg_truthtable[i].tag);

		if (!retval) {

			if (cfg_truthtable[i].tag_status == MANDATORY) {

				lprintf("%s mandatory tag %s is missing, rollback!\n", prefix, cfg_truthtable[i].tag);
				return 0;
			}

			continue;
		}

		if (!cfg_validate(i, retval)) {
			return 0;
		}

		if (cfg_truthtable[i].dep_tag) {

			retval = hashtable_lookup(c_cfg, cfg_truthtable[i].dep_tag);

			if (!retval || !strcmp(retval, "off")) {

				lprintf("%s you must enable %s tag inorder to use %s option, ",
					prefix, cfg_truthtable[i].dep_tag, cfg_truthtable[i].tag);

				switch (mode) {

					case ERROR_MODE:
						lraw("aborted!\n");
						return 0;
						break;

					case GLITCH_MODE:
						lraw("disabled!\n");
						hashtable_insert(c_cfg, cfg_truthtable[i].tag, "off");
						break;
				}
			}
		}
	}	

	build_cfg_bitmask_opts(c_cfg);

	return 1;
}

/*
 * cfg_typechk_string, check for valid string
 * * buf, given buffer
 * * len, given buffer length
 */

int cfg_typechk_string(char *buf, int len) {
	int j;

	for (j = 0; j < len; j++) {
		if (!isprint(buf[j])) {
			return 0;
		}
	}

	return 1;
}

/*
 * cfg_typechk_numeric, check for valid numeric
 * * buf, given buffer
 * * len, given buffer length
 */

int cfg_typechk_numeric(char *buf, int len) {
	int j;

	for (j = 0; j < len; j++) {
		if (!isdigit(buf[j])) {
			return 0;
		}
	}

	return 1;
}

/*
 * cfg_typechk_boolean, check for valid boolean
 * * buf, given buffer
 * * len, given buffer length
 */

int cfg_typechk_boolean(char *buf, int len) {

	if (!strcmp(buf, "on") || !strcmp(buf, "off")) {
		return 1;
	}

	return 0;
}

/*
 * cfg_typechk_url, check for valid HTTP URL
 * * buf, given buffer
 * * len, given buffer length
 */

int cfg_typechk_url(char *buf, int len) {
	int prev_tst;

	prev_tst = cfg_typechk_string(buf, len);

	if (!prev_tst || buf[len-1] != '/') {
		return 0;
	}

	return 1;
}

/*
 * config_dump, dump current configuration layout
 * * fp, given FILE pointer
 */

int config_dump(FILE *fp) {
	int i;
	char *retval;

	fprintf(fp, "# Ghost Configuration File\n");

	for (i = 0; cfg_truthtable[i].tag != NULL; i++) {

		retval = hashtable_lookup(cfg, cfg_truthtable[i].tag);
	
		if (retval) {
		
			fprintf(fp, "%s = %s\n", cfg_truthtable[i].tag, retval);
		}
	}

	fprintf(fp, "# EOF\n");

	return 1;
}
