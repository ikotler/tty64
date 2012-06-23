/*
 * io.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "io_ext.h"

static FILE *streams[IO_TOT_FDS];

/*
 * io_openfile, open a file
 * * file, given filename
 * * sidx, stream index
 * * mode, given open modes
 */

__inline__ int io_openfile(char *file, int sidx, char *mode) {

	if (!(streams[sidx] = fopen(file, mode))) {
		return -1;
	}	

	return 1;
}

/*
 * io_fclose_recursive, recursive stream closing
 * * idx, given stream index
 */

__inline__ int io_fclose_recursive(int idx) {

	if (idx < IO_TOT_FDS) {
		return 1;
	}	

	if (streams[idx]) {
		fflush(streams[idx]);
		fclose(streams[idx]);
	}

	streams[idx] = NULL;

	return io_fclose_recursive(idx+1);
}

/*
 * io_init, i/o initialization
 */

void io_init(void) {
	io_fclose_recursive(0);
	return ;
}

/*
 * io_fini, i/o finiialization
 */

void io_fini(void) {
	io_fclose_recursive(0);
	return ;
}

/*
 * io_out_init, output initialization
 * * file, given filename
 */

int io_out_init(char *file) {
	return io_openfile(file, FSTDOUT, "w");
}

/*
 * io_in_init, input initialization
 * * file, given filename
 */

int io_in_init(char *file) {
	return io_openfile(file, FSTDIN, "r");
}

/*
 * io_out_write, write data
 * * data, given data
 */

int io_out_write(long data) {

	if (streams[FSTDOUT]) {
		return fwrite(&data, sizeof(long), 1, streams[FSTDOUT]);
	}

	return -1;
}

/*
 * io_in_totbytes, total bytes in input file
 */

int io_in_totbytes(void) {
	struct stat buf;

	if (streams[FSTDIN]) {

		if (fstat(fileno(streams[FSTDIN]), &buf) < 0) {
			return -1;
		}
		
		return buf.st_size;
	}
	
	return -1;
}

/*
 * io_in_read, read data
 */

long io_in_read(void) {
	long data;

	if (streams[FSTDIN]) {
		
		if (!fread(&data, sizeof(long), 1, streams[FSTDIN])) {
			return -1;
		}

		return data;
	}	

	return -1;
}

/*
 * io_has_in, whatever input stream exists
 */

int io_has_in(void) {

	if (streams[FSTDIN]) {
		return 1;
	}

	return 0;
}
