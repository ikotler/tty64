TARGET	= ghostd
OBJS	= ./src/ghostd.o ./src/conf.o ./src/daemon.o ./src/hash.o ./src/log.o ./src/httpd.o ./src/http.o ./src/proxy.o ./src/lowlevel.o ./src/www.o
CC	= gcc
CFLAGS	= -O2 -g -Wall
LIBS	= -lpthread

all:	ghostd

ghostd: ${OBJS}
	${CC} ${CFLAGS} ${LIBS} ${OBJS} -o $@

.c.o:
	${CC} ${CFLAGS} -c $< -o $@

clean:
	/bin/rm -rf ghostd ./src/*.o

