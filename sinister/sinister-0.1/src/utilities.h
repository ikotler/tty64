/*
 * utilities.h
 */

void *stoa(char *);
void *addr_increase(void *, int);
void *addr_decrease(void *, int);

void addr_setdirection(int);

unsigned long addr_substruct(void *, void *);

int addr_cmp(void *, void *);
int addr_transdirection(char *);
