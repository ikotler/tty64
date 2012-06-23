/*
 * opts.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "opts_ext.h"

/*
 * opts_alloc_default, allocate opts struct and assign default values
 */

opts *opts_alloc_default(void) {
	opts *ptr;

	ptr = calloc(1, sizeof(struct sinister_opts));

	if (!ptr) {
		return NULL;
	}
	
	ptr->bir = 4;
	ptr->direction = -1;
	ptr->cmdparams = -1;

	return ptr;
}

/*
 * opts_free, free options strcture
 * * optz, given options structure
 */

void opts_free(opts *optz) {

	if (optz) {
		free(optz);
	}

	return ;
}

/*
 * opts_has_direction, check if direction option set
 * * opt, given options
 */

int opts_has_direction(opts *opt) {

	if (opt->direction < 0) {
		fprintf(stderr, "*** missing direction option, please use '-m' to set direction!\n");
		return 0;
	}

	return 1;
}

/*
 * opts_has_action, check if action option set
 * * opt, given options
 */

int opts_has_action(opts *opt) {

	if (!opt->action) {
		fprintf(stderr, "*** missing action option, select one of the ('-d','-p','-s') options!\n");
		return 0;
	}

	return 1;
}

/*
 * opts_has_cmdparams, check if enough command line parameters passed
 * * opt, given options
 */

int opts_has_cmdparams(opts *opt) {

	if (opt->cmdparams < 1) {
		fprintf(stderr, "*** missing program filename or pid parameter!\n");
		return 0;
	}

	return 1;
}

/*
 * opts_err_badaddr, report on bad address
 * * addr, given address
 */

void opts_err_badaddr(char *addr) {
	fprintf(stderr, "*** (%s): given address is malformed/invalid!\n", addr);
	return ;
}

/*
 * opts_err_actconflict, two actions been selected in the same time
 */

void opts_err_actconflict(void) {
	fprintf(stderr, "**** an action already been selected!\n");
	return ;
}
