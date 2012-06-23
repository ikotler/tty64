/*
 * lib.h
 */

int lib_load(libptr *);
int lib_splitmask(libptr *, char *);
int lib_makestack(libptr *, int, int, char **);

libptr *lib_allocate(void);
void lib_free(libptr *);

unsigned long lib_exec(libptr *);
