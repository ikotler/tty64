/*
 * crypt.c
 */

#include <string.h>

/*
 * crypt_xorbuf, crypt/xor buffer with key
 * * buf, given buffer
 * * len, given buffer length
 * * key, given key/passwd
 */

char *crypt_xorbuf(char *buf, int len, char *key) {
	int buf_idx, key_idx;

	key_idx = 0;

	for (buf_idx = 0; buf_idx < len; buf_idx++) {
	
		buf[buf_idx] ^= key[key_idx];

		key_idx++;

		if (key_idx == strlen(key)) {
			key_idx = 0;
		}
	}

	return buf;
}
