/* 
 * io_ext.h
 */

typedef struct io_stream io_stream;

struct io_stream {
	char *buf;
	char *fname;
	int buflen;
};
