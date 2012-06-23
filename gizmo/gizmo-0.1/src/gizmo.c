/*
 * gizmo.c, root of all evil
 * code by ik @ ikotler.org
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include <elf.h>

#include "gizmo.h"
#include "shbuf.h"
#include "shells.h"
#include "elf_ext.h"
#include "elf.h"
#include "gfx.h"
#include "misc.h"

int verbose, gizmos, asm_garbage;

int main(int argc, char **argv) {
	char c, *gizmo_type;
	int type, retval;

	gizmo_type = NULL;
	verbose = asm_garbage = 0;

	gizmos = shells_init();

	if (argc < 2) {
		usage(argv[0]);
		return 0;
	}

	if (!gizmos) {

		if (strcmp(argv[1], "-a") && strcmp(argv[1], "-h")) {

			fprintf(stderr, "*** this binary/version has been compiled without any gizmos in it!\n");
			return 0;
		}
	}

	while ((c = getopt (argc, argv, "gahvlt:")) != -1) {

        	switch (c) {

			case 'g':
				asm_garbage = 1;
				break;

			case 'a':

				about();
				return 0;
				break;

			case 'l':

				shells_foreach(shells_fulldesc);
				return 0;
				break;
			
			case 'h':

				usage(argv[0]);
				return 0;
				break;

			case 't':
				
				gizmo_type = strdup(optarg);
				break;

			case 'v':

				verbose = 1;
				break;

			default:
				return 0;
				break;
		}
	}

	type = isnumeric(gizmo_type);

	if (type < 0) {

		if (!gizmo_type) {
			fprintf(stderr, "*** no gizmo type was selected, please use '-t' to specify a gizmo type!\n");
		} else {
			fprintf(stderr, "*** specified gizmo type (%s) isn't a numeric value\n", gizmo_type);
		}

		return 0;
	}

	if (type > gizmos) {
		fprintf(stderr, "*** gizmo type is out of range (%d?, 0-%d), retry again!\n", type, gizmos);
		return 0;
	}

	retval = gizmo_elf_processor(argv[argc-2], argv[argc-1], shells_get(type));

	return retval;
}

/*
 * usage, display usage
 * * basename, barrowed argv[0]
 */

void usage(char *basename) {
	fprintf(stderr, "usage: %s [-vhlag] <-t gizmo type> <input> <output>\n\n", basename);

	fprintf(stderr, "options:\n");
	fprintf(stderr, "--------\n");
	fprintf(stderr, "\t -v, verbose / verbose mode\n");
	fprintf(stderr, "\t -h, usage / this usage screen\n");
	fprintf(stderr, "\t -t, gizmo type / specify gizmo type to use\n");
	fprintf(stderr, "\t -l, list / start the gizmos fashion show\n");
	fprintf(stderr, "\t -a, about / display version and quit\n");
	fprintf(stderr, "\t -g, asm garbage / pad with generated assembly garbage\n\n");

	if (gizmos) {

		fprintf(stderr, "gizmos:\n");
		fprintf(stderr, "-------\n");

		shells_foreach(shells_shrtdesc);

	} else {
	
		fprintf(stderr, "\t *** no gizmos compiled within this version/binary!\n");
	}

	fprintf(stderr, "\n");

	return ;
}

/*
 * about, display version/banner
 */

void about(void) {

	fprintf(stderr, "gizmo (v%s [%d gizmos]) <%s>\n\t--> http://www.ikotler.org / mailto:<ik@ikotler.org>\n", 
		GIZMO_VERSION, gizmos, __DATE__);

	return ;
}
