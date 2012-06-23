/*
 * asm.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

/*
 * gizmo_asmgen, generate x86 assembly garbage
 * * buf, given buffer
 * * len, given length
 */

void gizmo_asmgen(char *buf, int len) {
	int j;

	for (j = 0; j < len; j++) {
		srand((time(NULL) * (j+1))-j);
		buf[j] = (rand()+1) % 255;
	}

	return ;
}
