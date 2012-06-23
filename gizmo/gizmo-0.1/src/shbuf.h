/*
 * shbuf.h
 */

typedef struct shbuf shbuf;

struct shbuf {
	char *buf;
	int index;
};

shbuf *shbuf_create(int, char);
int shbuf_append(shbuf *, char *, int);
void shbuf_free(shbuf *);

