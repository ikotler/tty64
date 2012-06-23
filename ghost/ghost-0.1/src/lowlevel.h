/*
 * lowlevel.h
 */

char *net_getpeeraddr(int);

int net_mastersocket(char *, char *);
int net_acceptsocket(int);
int net_setononblock(int);
int net_opensocket(char *, int, int);

int async_read(int, char *, int, int);
int async_write(int, char *, int, int);

int async_duplex_bridge(int, int, void *(*)(char **, int), void *(*)(char **, int), char *, int);

