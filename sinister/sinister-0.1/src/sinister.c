/*
 * sinster.c, root of all evil
 * code by ik @ ikotler.org
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/ptrace.h>
#include <signal.h>

#include "opts_ext.h"
#include "opts.h"
#include "utilities.h"
#include "process_ext.h"
#include "process.h"
#include "io.h"
#include "hexdump.h"
#include "sinister.h"

#include "consts.h"

void usage(char *);
void about(void);
void ctrlc(int num);

int verbose;

extern int optind;
extern char *optarg;

int main(int argc, char **argv, char **envp) {
	process *app;
	opts *optz;
	int retval;
	char c;

	verbose = 0;

	if (argc < 2) {
		usage(argv[0]);
		return 0;
	}

	io_init();

	optz = opts_alloc_default();

	if (!optz) {
		perror("calloc");
		return -1;
	}

        while ((c = getopt (argc, argv, "ahvsdp::m:x:z:b:i:o:")) != -1) {

                switch (c) {

			case 'a':
		
				about();
				return 0;
				break;

			case 'h':

				usage(argv[0]);
				opts_free(optz);
				return 0;
				break;

			case 'v':

				verbose++;
				break;

			case 'x':

				if (!(optz->s_addr = stoa(optarg))) {
					opts_err_badaddr(optarg);
					opts_free(optz);
					return 0;
				}

				break;

			case 'z':
				
				if (!(optz->e_addr = stoa(optarg))) {
					opts_err_badaddr(optarg);
					opts_free(optz);
					return 0;
				}

				if (!addr_cmp(optz->s_addr, optz->e_addr)) {

					fprintf(stderr, "*** illegal range (%p - %p) within the choosen direction (%s)!\n",
						optz->s_addr, optz->e_addr, (optz->direction) ? "up" : "down");
					
					opts_free(optz);
					return 0;
				}

				break;
			
			case 'b':

				if (!(optz->bir = atoi(optarg))) {
					fprintf(stderr, "*** (%s) illegal numeric value!\n", optarg);
					opts_free(optz);				
					return 0;
				}

				break;

			case 'm':

				if ((optz->direction = addr_transdirection(optarg)) < 0) {
					fprintf(stderr, "*** illegal direction (%s), direction can be 'up' or 'down'\n", optarg);
					opts_free(optz);
					return 0;
				}

				if (!optz->e_addr || !optz->s_addr) {

					if (!optz->e_addr) {

						optz->e_addr = (void *) 0xffffffff;

						if (!optz->direction) {
							optz->e_addr = (void *) 0x00000000;
						}
					}

					if (!optz->s_addr) {

						optz->s_addr = (void *) 0x00000000;

						if (!optz->direction) {
							optz->s_addr = (void *) 0xffffffff;
						}
					}
				}

				addr_setdirection(optz->direction);

				break;
			
			case 'i':
	
				if (io_in_init(optarg) < 0) {
					perror(optarg);
					opts_free(optz);					
					return 0;
				}

				if (!io_in_totbytes()) {
					fprintf(stderr, "*** corrupted image file (%s) has %d byte size!\n", optarg, io_in_totbytes());
					opts_free(optz);					
					return 0;
				}

				if ( (io_in_totbytes() % PTRACE_WORD) != 0) {

					fprintf(stderr, "*** image (%s) filesize (%d) has to be multiplication of %d!\n", 
							optarg, io_in_totbytes(), PTRACE_WORD);

					opts_free(optz);
					return 0;
				}

				break;

			case 'o':

				if (io_out_init(optarg) < 0) {
					perror(optarg);
					opts_free(optz);
					return 0;
				}
				
				break;

			case 'd':

				if (!optz->action) {

		                        if (hexdump_initialization(optz->bir, optz->s_addr, optz->e_addr) < 0) {
	        	                        return 0;
	                	        }

					optz->action = PROCESS_DUMP;

				} else {
					opts_err_actconflict();
					return 0;
				}

				break;

			case 'p':

				if (!optz->action) {

					if (!io_has_in()) {
						fprintf(stderr, "*** missing input binary image, please use '-i' specify input image!\n");
						opts_free(optz);
						return -1;
					}

					optz->action = PROCESS_PATCH;

				} else {
					opts_err_actconflict();
					return 0;
				}

				break;
			
			case 's':

				if (!optz->action) {

					optz->action = PROCESS_SCAN;

				} else {
					opts_err_actconflict();
					return 0;
				}

				break;
		}
	}

	optz->cmdparams = argc - optind;

	if ( (!opts_has_direction(optz)) || 
	     (!opts_has_action(optz))    ||
	     (!opts_has_cmdparams(optz)) )
	{
		opts_free(optz);
		return 0;
	}

	if ( (optz->cmdparams == 1) && (atoi(argv[optind])) ) {
 
		app = process_attach(atoi(argv[optind]));

	} else {

		app = process_create(argv[optind], process_argv_maker(optind, argv, argc), envp);

	}

	if (!app) {
		opts_free(optz);
		return 0;
	}

	signal(SIGINT, ctrlc);

	process_setcur(app);

	retval = process_start(app, optz);

	io_fini();

	process_destory(app);

	opts_free(optz);

	return retval;
}

/*
 * usage, display usage screen
 * * basename, borrowed argv[0]
 */

void usage(char *basename) {
	fprintf(stderr, "usage: %s <options> [ <program> [<program parameters>] | <pid> ]\n\n", basename);

	fprintf(stderr, "options:\n");
	fprintf(stderr, "--------\n");

	fprintf(stderr, "\t -a, about / banner and quit\n");
	fprintf(stderr, "\t -v, verbose / verbose mode\n");
	fprintf(stderr, "\t -h, usage / show this screen\n");
	fprintf(stderr, "\t -x, addr / initial memory address\n");
	fprintf(stderr, "\t -z, addr / finial memory address\n");
	fprintf(stderr, "\t -m, movement / (up|down) direction\n");
	fprintf(stderr, "\t -d, dump / memory dump\n");
	fprintf(stderr, "\t -p, patch / memory patch\n");
	fprintf(stderr, "\t -s, scan / memory map\n");
	fprintf(stderr, "\t -i, input / binary input file\n");
	fprintf(stderr, "\t -o, output / binary output file\n");
	fprintf(stderr, "\t -b, bytes / # of bytes in a row\n\n");

	return ;
}

/*
 * about, version display
 */

void about(void) {

	printf("Sinister <v%s> (mailto:ik@ikotler.org [or] http://www.ikotler.org) | [WORD alignment %d bytes]\n",
		SINISTER_VERSION, PTRACE_WORD);

	return ;
}

/*
 * ctrlc, signal handler
 * signum, signal #
 */

void ctrlc(int signum) {
	process *cur;

	fputc('\n', stdout);

	cur = process_getcur();

	if (cur) {
		process_destory(cur);
	}

	if (verbose) {
		fprintf(stdout, "*Interrupt!*\n");
	}

	exit(0);

	/* NEVER REACH */
	return ;
}
