/*
 * lib.c
 */

#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <dlfcn.h> 
#include <string.h> 
#include <errno.h> 
#include <signal.h>

#include <sys/mman.h>

#include "io.h" 
#include "lib_ext.h" 
#include "gfx.h"

extern int verbose;

/*
 * lib_exec_safetybelt, signal trap for lib_exec
 * * signum, catched signal #
 */

void lib_exec_safetybelt(int signum) {

	io_fastrelease();

	if (verbose) {

		fprintf(stderr, "\n *** Caught SIGNAL #%d (SIGSEGV) ***\n\n", signum);

	} else {

		fputc('\n', stdout);
		gfx_box("Caught SIGNAL #%d (SIGSEGV)", signum);

		printf(\
			"This normally happens as the result of an incorrect/lack of parameters\n" \
			"make sure that parameters that are pointers are wrapped with double quotes (eg. \"\")\n" \
			"and the right amount/order of parameters are being passed to the function\n\n" \
			);
	}

	exit(-1);

	/* NEVER REACH */
	return ; 
}

/*
 * lib_isptr, check if given value is pointer
 * * addr, given address
 */

int lib_isptr(unsigned long addr) {
	int retval;

	retval = mprotect( (void *) (addr & 0xFFFFF000) , 4096, PROT_READ | PROT_WRITE);

	if (retval < 0) {
		return -1;
	}

	return strlen((char *)addr); 
}

/*
 * __cutnpaste, take strtok() result and strdup() it back
 * * curvar, current 'logical' part
 * * str, given text
 */

static __inline__ char * __cutnpaste(char *curvar, char *str) {
	char *ptr;

	if (!str) {
		fprintf(stderr, "*** invalid mask, can't find %s name!\n", curvar);
		return NULL;
	}

	ptr = strdup(str);

	if (!ptr) {
		perror("strdup");
		return NULL;
	}

	return ptr; 
}

/*
 * __lib_dlaction, error-aware wrapper for dl() family functions
 * * res, given function result
 */

static __inline__ void * __lib_dlaction(void *res) {
	char *errmsg;
	
	if (!res && (errmsg = dlerror())) {
		fprintf(stderr, "*** %s\n", errmsg);
	}
	
	dlerror();

	return res; 
}

/*
 * lib_spiltmask, split <lib,func> mask
 * * ptr, given library pointer
 * * mask, given mask
 */

int lib_splitmask(libptr *ptr, char *mask) {
	char *dupmask;

	dupmask = strdup(mask);

	if (!dupmask) {
		perror("strdup");
		return 0;
	}

	ptr->lib_name = __cutnpaste("library"  , strtok(dupmask, ","));
	ptr->fcn_name = __cutnpaste("function" , strtok(NULL, ","));

	free(dupmask);

	if (!ptr->lib_name || !ptr->fcn_name) {
		return 0;
	}

	return 1; 
}

/*
 * lib_load, load library and resolv the symbol
 * * ptr, given library pointer
 */

int lib_load(libptr *ptr) {

	/*
	 * Open handler to library and resolv the symbol/function
	 */

	ptr->lib_handler = __lib_dlaction(dlopen(ptr->lib_name, RTLD_LAZY));
	ptr->fcn_handler = __lib_dlaction(dlsym(ptr->lib_handler, ptr->fcn_name));

	if (!ptr->lib_handler || !ptr->fcn_handler) {
		return 0;
	}

	/*
	 * It's the only way i've figured out to get back the selected library (full path)
 	 */

	dlsym(ptr->lib_handler, "_f00bar$%_");
	ptr->lib_path = strtok(dlerror(), ":");

	if (!ptr->lib_path) {
		ptr->lib_path = ptr->lib_name;
	}

	if (verbose) {
		gfx_box("%s[<0x%08x>]@%s[<%s>]", ptr->fcn_name, ptr->fcn_handler, ptr->lib_name, ptr->lib_path);
	}

	return 1; 
}

/*
 * lib_makestack, create stack for our function
 * * lptr, given library pointer
 * * sidx, initial index in the vec
 * * eidx, finial index in the vec
 * * vec, given parameters vector
 */

int lib_makestack(libptr *ptr, int sidx, int eidx, char **vec) {
	int totitems, j;
	void *data;

	totitems = eidx - sidx;

	/*
	 * Allocate our function stack
	 */

	if ( (!(ptr->stack = calloc(1, sizeof(struct __libptr_stack)))) ||
	     (!(ptr->stack->stack = calloc(totitems, sizeof(long)))))
	{
		if (ptr->stack) {
			free(ptr->stack);
		}
		
		perror("calloc");

		return 0;
	}

	ptr->stack->stack_items = totitems;

	/*
 	 * Generate function stack layout
	 */

	for (j = 0; eidx > sidx; eidx--) {

	        if (vec[eidx][0] == '"') {

        	        vec[eidx] += 1;
                	vec[eidx][strlen(vec[eidx]) - 1] = '\0';
			data = (void *) vec[eidx];

                } else {

			data = (unsigned long *) strtoul(vec[eidx], NULL, 0);

                }

		memcpy((void *)&ptr->stack->stack[j], (void *)&data, sizeof(long));

		j++;
	}

	if (verbose) {
		int txtlen;

		txtlen = (printf("\t* Stack Generated (%d parameters, %d bytes)\n\t", totitems, totitems * sizeof(long))-3);
		gfx_hr(txtlen);

		fputc('\n', stdout);
	}

	return 1; 
}

/*
 * lib_exec, execute function inside the library
 * * ptr, given library pointer
 */

unsigned long lib_exec(libptr *ptr) {
	unsigned long ret;
	int j, rval, s_errno;

	if (verbose) {

		/*
		 * Display psuedo-generated Assembly
		 */

		printf("\tGenerated Assembly\n\t");
		gfx_hr(18);

		fputc('\n', stdout);

		if (ptr->stack) {

			for (j = 0; j < ptr->stack->stack_items; j++) {
				printf("\t\t * pushl $0x%08x\n", (unsigned int) ptr->stack->stack[j]);
			}
		}
	
		printf("\t\t * call %p\n\n", ptr->fcn_handler);
	}

	/*
	 * Executing Procedure:
	 *
	 * * Registering to SIGSEGV Signal, in case something goes wrong
	 *   It would be more appropriate to catch it and exit normally
	 *
	 * * Bufering up STDOUT & STDERR
	 *
	 * * Manually reset the 'errno' variable, for induction if the
	 *   function has silently failed and gave us error via errno.
	 *
	 * * Manually pushing the arguments to the stack
	 *
	 * * CALL the chosen function, and keep it's return value!
 	 *
	 * * Cleaning up the stack after the CALL
	 *
	 * * Saving up current 'errno' value
	 *
	 * * Restoring SIGSEGV Signal to it's default handler
	 *
	 * * Releasing the STDOUT & STDERR buffers
 	 *
 	 */

	signal(SIGSEGV, (void *)lib_exec_safetybelt);

	io_buffer();

	errno = 0;
	
	/*
	 * Manually pushing the function arguments to the stack
 	 */

	if (ptr->stack) {

		for (j = 0; j < ptr->stack->stack_items; j++) {

			__asm__ __volatile__ (\
				"pushl %0 \n" \
				: /* no output */ \
				: "r" (ptr->stack->stack[j]) \
				: "%eax" \
				);
		}
	}

	/*
	 * Make the CALL!
	 */

	ret = (unsigned long) ptr->fcn_handler();

	/*
	 * Be polite, let's clean the stack afterward
	 */

	if (ptr->stack) {

		ptr->stack->stack_items *= sizeof(long);

		__asm__ __volatile__ (\
			"addl %0, %%esp \n" \
			: /* no output */ \
			: "r" (ptr->stack->stack_items) \
			: "%esp" \
			);

		ptr->stack->stack_items /= sizeof(long);
	}

	s_errno = errno;

	signal(SIGSEGV, SIG_DFL);

	io_release();

	if (verbose) {
	
		char *stdout_buf, *stderr_buf;

		stdout_buf = io_get_stdout();
		stderr_buf = io_get_stderr();
		
		printf("\tStreams Buffers\n\t");
		gfx_hr(15);
		fputc('\n', stdout);

		if (!stdout_buf && !stderr_buf) {

			printf("\t\t * Empty Buffers (STDOUT/STDERR)\n\n");

		} else {

			printf(\
				"\t\t * Standart Output (STDOUT) : %d bytes\n" \
				"\t\t * Standart Error  (STDERR) : %d bytes\n\n",
				((stdout_buf) ? strlen(stdout_buf) : 0),
				((stderr_buf) ? strlen(stderr_buf) : 0) );
		}
	}

	if (verbose) {

		printf("\tFunction Result\n\t");
		gfx_hr(15);
		fputc('\n', stdout);
	}

	if (s_errno) {

		if (verbose) {

			fprintf(stderr, \
				"\t\t * Error Number  : #%d\n" \
				"\t\t * Error Message : %s\n\n", \
				(int)ret, strerror(s_errno));

		} else {

			fprintf(stderr, "(%d, %s)\n", (int)ret, strerror(s_errno));
		}

		rval = -1;

	} else {

		/*
		 * Let's examine if the function gave us back a pointer
		 */

		rval = lib_isptr(ret);

		if (rval > -1) {

			if (verbose) {

				fprintf(stdout, \
					"\t\t * Pointer: Yes\n" \
					"\t\t * Address: 0x%08x\n" \
					"\t\t * Value: %s\n\n", \
					(unsigned int) ret, (rval) ? (char *) ret : "non-ascii-content");

			} else {

				printf("(0x%08x) [%s]\n", (unsigned int) ret, (rval) ? (char *) ret : (char *) "non-ascii-content");

			}

		} else {

			if (verbose) {

				fprintf(stdout, \
					"\t\t * Pointer: No\n" \
					"\t\t * Value: %d\n\n", \
					(int)ret);

			} else {

				printf("(%d)\n", (int)ret);
		
			}
		}

		rval = 1;
	}

	io_killbufs();

	ptr->fcn_rval = (int) ret;

	return rval; 
}

/*
 * lib_allocate, allocate library pointer structure
 */

libptr *lib_allocate(void) {
	return calloc(1, sizeof(libptr)); 
}

/*
 * lib_free, free()'s library pointer structure
 * * ptr, given library pointer
 */

void lib_free(libptr *ptr) {

	if (ptr) {

		if (ptr->fcn_name) {
			free(ptr->fcn_name);
			free(ptr->lib_name);
		}

		if (ptr->lib_handler) {
			dlclose(ptr->lib_handler);
		}

		if (ptr->stack) {
			free(ptr->stack->stack);
			free(ptr->stack);
		}

		free(ptr);
	}

	return ; 
}
