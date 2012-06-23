/*
 * misc.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

/*
 * getpasswd, prompts for password
 * * str, given string for prompt
 */

char *getpasswd(char *str) {
        char *ptr, *passwd;

        while(1) {

                ptr = getpass(str);

                if (!ptr) {
                        fprintf(stderr, "* INVALID PASSWORD, RETRY AGAIN!\n");
                        continue;
                }

                if (strlen(ptr) < 2 || strlen(ptr) > 17) {
                        fprintf(stderr, "* INVALID PASSWORD, LENGTH IS EITHER SHORTER THEN 1 BYTE OR LONGER THEN 16 BYTES, RETRY AGAIN!\n");
                        continue;
                }

                passwd = strdup(ptr);

                if (!passwd) {
                        perror("strdup");
                        return NULL;
                }

                memset(ptr, 0x0, strlen(passwd));

                break;
        }

        return passwd;
}

/*
 * passwdloop, prompts for passwd then verify it
 */

char *passwdloop(void) {
	char *passwd, *v_passwd;

        while(1) {

                passwd = getpasswd("* ENTER PASSWORD: ");

                if (!passwd) {
			break;
                }

                v_passwd = getpasswd("* ENTER PASSWORD AGAIN: ");

                if (!v_passwd) {
                        free(passwd);
                        return NULL;
                }

                if (!strcmp(passwd, v_passwd)) {
                        free(v_passwd);
                        break;
                }

                fprintf(stderr, "*** PASSWORDS DOES NOT MATCH, RETRY AGAIN! ***\n");

                free(passwd);
                free(v_passwd);
        }

	return passwd;
}

/*
 * isnumeric, check if given string is numeric
 * * str, given string
 */

int isnumeric(char *str) {
	int j, len;

	if (!str) {
		return -1;
	}

	len = strlen(str);

	for (j = 0; j < len; j++) {
		if (!isdigit(str[j])) {
			return -1;
		}
	}

	return atoi(str);
}
