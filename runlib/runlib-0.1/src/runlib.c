/*
 * runlib, the root of all evil
 * code by ik @ ikotler.org
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "io.h"
#include "runlib.h"
#include "lib_ext.h"
#include "lib.h"

void usage(char *);
void about(void);

extern int optind;
int verbose;

int main(int argc, char **argv) {
	int c, retval, rtype;
	libptr *lptr;

	verbose = rtype = 0;

	if (argc < 2) {
		usage(argv[0]);
		return 0;
	}

        while ((c = getopt (argc, argv, "ahvrz:x:")) != -1) {

		switch (c) {

			case 'v':
				verbose++;
				break;
	
			case 'h':
				usage(argv[0]);
				return 0;
				break;

			case 'a':
				about();
				return 0;
				break;

			case 'r':
				rtype = 1;
				break;

			case 'x':
				io_foutput(0 , optarg);
				break;

			case 'z':
				io_foutput(1, optarg);
				break;

		}
	}

	if (!argv[optind]) {
		fprintf(stderr,"*** missing mask (eg. [<lib,func>]) in arguments!\n");
		return 0;
	}

	if (!strstr(argv[optind], ",")) {
		fprintf(stderr, "*** (%s): no delimiter (',') found in given mask\n", argv[optind]);
		return 0;
	}

	if (!(lptr = lib_allocate())) {
		perror("calloc");
		return 0;
	}

	if ( ( (!lib_splitmask(lptr, argv[optind])) ) ||
	     ( (!lib_load(lptr)) ) )
	{
		return 0;
	}

	if (((argc-1) - optind)) {
		
		if (lib_makestack(lptr, optind, argc-1, argv) < 0) {
			return 0;
		}

	}

	retval = lib_exec(lptr);

	if (rtype) {

		retval = (int)lptr->fcn_rval;

	}

	lib_free(lptr);

	return retval;
}

/*
 * usage, display usage screen
 * * basename, barrowed argv[0]
 */

void usage(char *basename) {
	fprintf(stderr, "usage: %s [<options>] <lib,func> [<function parameters>]\n\n", basename);
	
	fprintf(stderr, "options:\n");
	fprintf(stderr, "--------\n");

	fprintf(stderr, "\t -a, about / banner and quit\n");
	fprintf(stderr, "\t -v, verbose / verbose mode\n");
	fprintf(stderr, "\t -h, usage / show this screen\n");
	fprintf(stderr, "\t -r, retval / use func retval\n");
	fprintf(stderr, "\t -x, stdout / func stdout output\n");
	fprintf(stderr, "\t -z, stderr / func stderr output\n\n");

	return ;
}

/*
 * about, display about message
 */

void about(void) {
	printf("runlib (v%s) <mailto:ik@ikotler.org [or] http://www.ikotler.org>\n", RUNLIB_VERSION);
	return ;
}
