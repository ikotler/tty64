/*
 * hexdump.h
 */

int hexdump_initialization(int, void *, void *);
void hexdump_finialization(void);

int hexdump_push(long);
int hexdump_display(void);

