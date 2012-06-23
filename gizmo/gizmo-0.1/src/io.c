/*
 * io.c
 */

#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>

/*
 * io_getchunk, get chunk of #(length) data bytes from offset
 * * fp, given FILE pointer
 * * offset, given offset
 * * length, given length
 */

char *io_getchunk(FILE *fp, long offset, int length) {
	void *chunk;

	chunk = calloc(1, length);

	if (!chunk || fseek(fp, offset, SEEK_SET) < 0 || fread(chunk, length, 1, fp) < 0) {
		
		if (chunk) {
			free(chunk);
		}

		return NULL;
	}

	return chunk;
}

/*
 * io_chunkcpy, copy chunk of #(length) data bytes from src to dst
 * * src, given source FILE pointer
 * * dst, given destnation FILE pointer 
 * * offset, given offset in source
 * * length, given length
 * * pagesize, given page size
 */

int io_chunkcpy(FILE *src, FILE *dst, long offset, int length, int pagesize) {
	void *buf;
	int pages = 0;

	buf = calloc(1, pagesize);

	if (!buf || fseek(src, offset, SEEK_SET) < 0) {

		if (buf) {
			free(buf);
		}

		return 0;
	}

	while ( pages + pagesize  < length) {
		
		if ( (!fread(buf, pagesize, 1, src)) || (!fwrite(buf, pagesize, 1, dst)) )
		{
			free(buf);
			return 0;
		}

		pages += pagesize;
	}

	if ( (!fread(buf, length - pages, 1, src)) || 
		(!fwrite(buf, length - pages, 1, dst)) )
	{
		free(buf);
		return 0;
	}

	free(buf);

	return 1;
}

/*
 * io_filesize, return file size
 * * fp, given FILE pointer
 */

int io_filesize(FILE *fp) {
	struct stat tmp;

	if (fstat(fileno(fp), &tmp) < 0) {
		return -1;
	}

	return tmp.st_size;
}
