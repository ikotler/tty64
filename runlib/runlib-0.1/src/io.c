/*
 * io.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "io_ext.h"

/*
 * File descriptors table
 */

static int spipes[2][2] = {
	{ -1 , -1 },
	{ -1 , -1 }
};

static int opipes[] = { 1, 2 };

/*
 * Stream buffers table
 */

static io_stream streamsbufs[] = {
	{ NULL, NULL , -1 },
	{ NULL, NULL , -1 }
};

/*
 * __io_getbuf, return pointer to given buffer
 * * bindex, buffer index
 */

__inline__ static char *__io_getbuf(int bindex) {
	char *ptr;

	ptr = streamsbufs[bindex].buf;

	return ptr;
}

/*
 * __io_foreach, abstract loop
 * * fcn, given function
 * * ite, no # of iteration
 */

__inline__ static void __io_foreach(void (*fcn)(int), int ite) {
	int j;

	for (j = 0; j < ite; j++) {
		fcn(j);
	}

	return ;
}

/*
 * io_bufpipe, dump pipe buffer to given buffer
 * * fd, given pipe fd
 * * bidx, given buffer index
 */

void io_bufpipe(int fd, int bidx) {
	char tmpbuf[1024];
	struct timeval tv;
	int rval;
	fd_set rfd;	

        FD_ZERO(&rfd);
        FD_SET(fd, &rfd);

	tv.tv_sec = 0;
	tv.tv_usec = 5;

	rval = select(fd+1, &rfd, NULL, NULL, &tv);

	if (rval <= 0) {
		return ;
	}

	memset(tmpbuf, 0x0, sizeof(tmpbuf));

	rval = read(fd, tmpbuf, sizeof(tmpbuf));

	if (rval < 0) {
		return ;
	}

	streamsbufs[bidx].buf = calloc(rval, sizeof(char));

	if (streamsbufs[bidx].buf) {

		memcpy(streamsbufs[bidx].buf, tmpbuf, rval);

		streamsbufs[bidx].buflen = rval;
	}

	return ;
}

/*
 * io_bufferpipe, start buffering a given pipe, and redirect the pipe
 * idx, given stream index
 */

void io_bufferpipe(int idx) {

	opipes[idx] = dup(idx+1);

	if (!(pipe(spipes[idx]))) {

		dup2(spipes[idx][1], idx+1);

	}

	return ;
}

/*
 * io_buffer, start buffering by redirecting streams
 */

void io_buffer(void) {

	__io_foreach(io_bufferpipe, 2);

	return ;
}

/*
 * io_releasepipe, stop buffering a given pipe, and readjust to original output
 * * idx, given stream index
 */

void io_releasepipe(int idx) {
	int fd;
	
	if (spipes[idx][0] > 0) {

		close(spipes[idx][1]);

		if (streamsbufs[idx].fname) {

			io_bufpipe(spipes[idx][0], idx);

		}

		close(spipes[idx][0]);

		if (streamsbufs[idx].buf && streamsbufs[idx].fname) {
		
			fd = open(streamsbufs[idx].fname, O_CREAT|O_WRONLY);

			if (fd > 0) {

				write(fd, streamsbufs[idx].buf, streamsbufs[idx].buflen);

				close(fd);

			}			
		}

		dup2(opipes[idx], idx+1);
	}
	
	return ;
}

/*
 * io_release, stop buffering and readjust redirections
 */

void io_release(void) {

	fflush(stdout);
	fflush(stderr);

	__io_foreach(io_releasepipe, 2);

	return ;
}

/*
 * io_fastrelease, fast release -- readjust pipes
 */

void io_fastrelease(void) {

	dup2(opipes[0], 1);
	dup2(opipes[1], 2);

	return ;
}

/*
 * io_foutput, assign filename to stream
 * * idx, given stream index
 * * file, given output filename
 */

int io_foutput(int idx, char *file) {

	if (!streamsbufs[idx].fname) {

		streamsbufs[idx].fname = strdup(file);

	}

	return 1;
}

/*
 * io_killbuf, kill given stream buffer
 * * idx, given stream index
 */

void io_killbuf(int idx) {

	if (streamsbufs[idx].buf) {

		free(streamsbufs[idx].buf);

	}

	if (streamsbufs[idx].fname) {

		free(streamsbufs[idx].fname);

	}

	return ;
}

/*
 * io_killbufs, kill (release) stream buffers
 */

void io_killbufs(void) {

	__io_foreach(io_killbuf, 2);

	return ;
}

/*
 * io_get_stdout, get stdout buffer stream
 */

char *io_get_stdout(void) {

	return __io_getbuf(0);

}

/*
 * io_get_stderr, get stderr buffer stream
 */

char *io_get_stderr(void) { 

	return __io_getbuf(1); 

}
