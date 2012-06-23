/*
 * gfx.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>

/*
 * gfx_hr, create ascii horizontal line
 * * len, given border length
 */

void gfx_hr(int len) {
	int j;

	for (j = 0; j < len; j++) {
		fputc('-', stdout);
	}

	fputc('\n', stdout);

	return ;
}

/*
 * gfx_box, create ascii box for given text
 * * fmt, given fmt string
 */

void gfx_box(char *fmt, ...) {
        va_list ap;
        char buf[1024];

        va_start(ap, fmt);
        vsnprintf(buf, sizeof(buf), fmt, ap);
        va_end(ap);

	gfx_hr(strlen(buf));
	printf("%s\n", buf);
	gfx_hr(strlen(buf));
	fputc('\n', stdout);

	return ;
}
