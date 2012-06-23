OBJS	= ./src/gizmo.o ./src/elf.o ./src/io.o ./src/shells.o ./src/gfx.o ./src/misc.o ./src/asm.o ./src/preprocs.o ./src/shbuf.o ./src/crypt.o ./src/postprocs.o
CC	= gcc
CFLAGS	= -O2 -g -Wall

all:	gizmo

gizmo:  ${OBJS}
	${CC} ${CFLAGS} ${OBJS} -o $@

.c.o:
	${CC} ${CFLAGS} -c $< -o $@

clean:
	/bin/rm -rf gizmo ./src/*.o


