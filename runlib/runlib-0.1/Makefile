OBJS	= ./src/runlib.o ./src/lib.o ./src/gfx.o ./src/io.o
CC	= gcc
CFLAGS	= -O2 -g -Wall

#
# Linux require you keep it '-ldl', but *BSD isn't
#

LIBS	= -ldl

all:	runlib

runlib:  ${OBJS}
	${CC} ${CFLAGS} ${LIBS} ${OBJS} -o $@

.c.o:
	${CC} ${CFLAGS} -c $< -o $@

clean:
	/bin/rm -rf runlib ./src/*.o

