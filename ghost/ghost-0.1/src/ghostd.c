/*
 * ghostd.c, root of all evil
 * code by ik @ ikotler.org
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#include "hash.h"	
#include "conf.h"
#include "daemon.h"
#include "proxy.h"
#include "log.h"
#include "ghostd.h"

hashtable *cfg;

void usage(char *);
void lbanner(void);
void ctrlc();

int main(int argc, char **argv) {
	int retval;

        signal(SIGINT, (void *)ctrlc);

	if (argc < 2) {
		usage(argv[0]);
		return 0;
	}

	cfg = cfg_parse(argv[1]);

	if (!cfg || !cfg_validate_all(cfg, ERROR_MODE)) {
		return 0;
	}

	if (log_open()) {
		lbanner();
	}

	if (!proxy_parse()) {
		return 0;
	}

	retval = fork();

	if (retval < 0) {

		perror("fork");
		return 0;

	} else {

		if (!retval) {

			daemon_start();

			fprintf(stderr, "*** An error occurred. Please check %s for more details! ***\n", 
				(char *) hashtable_lookup(cfg, "daemon-logfile"));


			log_close();

			_exit(1);
		}
	}

	return 1;
}

/* 
 * usage, display usage
 * * basename, barrowed argv[0]
 */

void usage(char *basename) {
	fprintf(stderr, "ghost (v%s) <mailto:ik@ikotler.org>\n", GHOST_VERSION);
	fprintf(stderr, "usage: %s <cfgfile>\n", basename);
	return ;
}

/*
 * lbanner, log banner version to logfile
 */

void lbanner(void) {
        lprintf("[ghost/notice] --- Ghost (v%s) Started! ---\n", GHOST_VERSION);
	return ;

}

/*
 * ctrlc, signal hijack
 */

void ctrlc() {
        exit(1);
}
