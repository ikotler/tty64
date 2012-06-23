/*
 * log.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#include "hash.h"

extern hashtable *cfg;

pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

static FILE *log_fp;
static char *logfile;

/*
 * log_close, close file and end logging
 */

void log_close(void) {

	/* CRITICAL SECTION */
	pthread_mutex_lock(&log_mutex);

	if (log_fp) {
		fclose(log_fp);
		log_fp = NULL;
	}

	pthread_mutex_unlock(&log_mutex);
	/* CRITICAL SECTION */
	
	return ;
}

/*
 * curtime, return's current time
 */

__inline__ char *curtime(void) {

        time_t now;
        char *str_time;

        now = time(NULL);

        str_time = ctime(&now);
        str_time[strlen(str_time)-1] = '\0';

        return str_time;
}

/*
 * lprintf, log writer
 * * fmt, format string
 */

void lprintf(char *fmt, ...) {
        va_list ap;
        char buf[1024], *prefix;

	FILE *out;

	/* CRITICAL SECTION */
	pthread_mutex_lock(&log_mutex);

        va_start(ap, fmt);
        vsnprintf(buf, sizeof(buf), fmt, ap);
        va_end(ap);

	prefix = curtime();
	out = log_fp;

	if (!out) {
		prefix = "***";
		out = stdout;
	}
	
        fprintf(out, "[%s] %s", prefix, buf);
        fflush(out);

	pthread_mutex_unlock(&log_mutex);
	/* CRITICAL SECTION */

        return ;
}

/*
 * lerror, log system error
 * * str, who's to blame?
 */

void lerror(char *str) {
	lprintf("%s: %s\n", str, strerror(errno));
	return ;
}

/*
 * lraw, log raw string into log file
 * * str, given string
 */

void lraw(char *str) {
	fprintf(log_fp, str);
}

/*
 * log_open, open logfile and start logging
 */

int log_open(void) {

	/* CRITICAL SECTION */
	pthread_mutex_lock(&log_mutex);

	if (!logfile) {
		logfile = hashtable_lookup(cfg, "daemon-logfile");
	}

	if (!log_fp) {

		log_fp = fopen(logfile, "a+");
		
		if (!log_fp) {
			perror(logfile);
			pthread_mutex_unlock(&log_mutex);
			return 0;
		}
	}

	pthread_mutex_unlock(&log_mutex);
	/* CRITICAL SECTION */

	return 1;
}

/*
 * log_restart, remove current log file and resume logging
 */

int log_restart(void) {
	int retval;

	log_close();

	retval = remove(logfile);

 	if (retval == 0) { 
		retval = log_open();
	}

	return retval;
}
