OBJS	= ./src/sinister.o ./src/process.o ./src/hexdump.o ./src/utilities.o ./src/gfx.o ./src/io.o ./src/opts.o ./src/procfs_read.o
CC	= gcc
CFLAGS	= -O2 -g -Wall

all:	sinister

sinister:  ${OBJS}
	${CC} ${CFLAGS} ${OBJS} -o $@

.c.o:
	${CC} ${CFLAGS} -c $< -o $@

clean:
	/bin/rm -rf sinister ./src/*.o


