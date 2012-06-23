/*
 * utilities.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

static int direction;

/*
 * stoa (string to address)
 * * str, given string
 */

void *stoa(char *str) {
	return (void *) strtoul(str, NULL, 16);
}

/*
 * addr_increase, realtive address increasement
 * * addr, given address
 * * bytes, given # of bytes to incerase by
 */

void *addr_increase(void *addr, int bytes) {

	if (direction) {
		return (addr + bytes);
	}

	return (addr - bytes);
}

/*
 * addr_decrease, realtive address decreasement
 * * addr, given address
 * * bytes, given # of bytes to decerase by
 */

void *addr_decrease(void *addr, int bytes) {

	if (direction) {
		return (addr - bytes);
	}

	return (addr + bytes);
}

/*
 * addr_cmp, realtive address compare
 * * src, given initial address
 * * dst, given finial address
 */

int addr_cmp(void *src, void *dst) {

	if (direction) {
		return (src < dst);
	}	

	return (dst < src);
}

/*
 * addr_substruct, realtive address substruct
 * * src, given initial address
 * * dst, given final address
 */

unsigned long addr_substruct(void *src, void *dst) {

	if (direction) {
		return (dst - src);
	}	

	return (src - dst);
}

/*
 * addr_transdirection, translate string into direction flag
 * * str, given direction string
 */

int addr_transdirection(char *str) {
	int retval = 1;

	if (strcmp(str, "up") && strcmp(str, "down")) {
		return -1;
	}

	if (str[0] == 'd') {
		retval = 0;
	}

	return retval;
}

/*
 * addr_setdirection, set direction flag
 * * flag, new direction value
 */

void addr_setdirection(int flag) {
	direction = flag;
	return ;
}
