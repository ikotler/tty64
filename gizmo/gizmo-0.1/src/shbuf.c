/*
 * shbuf.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "shbuf.h"

/*
 * shbuf_create, create shellcode buffer
 * * len, given shellcode length
 * * initchr, character to fill up the buffer
 */

shbuf *shbuf_create(int len, char initchr) {	
	shbuf *tmp;

	tmp = calloc(1, sizeof(shbuf));

	if (!tmp) {
		perror("calloc");
		return NULL;
	}	
	
	tmp->buf = calloc(1, len);

	if (!tmp->buf) {
		perror("calloc");
		free(tmp);
		return NULL;
	}

	memset(tmp->buf, initchr, len);

	return tmp;
}

/*
 * shbuf_free, free shellcode buffer
 */

void shbuf_free(shbuf *buf) {
	
	if (buf) {

		if (buf->buf) {	
			free(buf->buf);
		}

		free(buf);
	}

	return ;
}

/*
 * shbuf_append, appends data into given shellcode buffer
 * * buf, given buffer
 * * data, given data to push
 * * len, given data length
 */

int shbuf_append(shbuf *buf, char *data, int len) {
	char *ptr;

	if (!buf || !buf->buf) {
		return 0;
	}

	ptr = (char *)&buf->buf[buf->index];

	memcpy(ptr, data, len);

	buf->index += len;

	return buf->index;
}
