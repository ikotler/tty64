BINS	= ./helloworld.o
NASM	= nasm
NFLAGS	= -f bin

all:	${BINS}
	
.s.o:
	${NASM} ${NFLAGS} $< -o $@
	chmod +x  $@
	mv $@ `echo $@ | sed 's/\.o/\.bin/g'`

clean:
	/bin/rm -rf *.bin

