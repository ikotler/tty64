/*
 * http.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifndef __USE_GNU
#define __USE_GNU
#endif

#include <string.h>

#include "log.h"
#include "http_ext.h"

/*
 * http_get_hdrsize, return HTTP header size from given http chunk
 * * chunk, given http chunk
 */

int http_gethdrsize(char *chunk) {
	char *greenmile;

	if (!chunk) {
		return -1;
	}

	greenmile = strstr(chunk, "\r\n\r\n");

	if (!greenmile) {
		return -1;
	}
	
	return ((greenmile+4) - chunk);
}

/*
 * is_http_header, check if given chunk is HTTP header
 * * chunk given chunk
 * * len, given length
 */

int is_http_header(char *chunk, int len) {
	
	if (!strstr(chunk, "HTTP/")) {
		return 0;
	}

	return 1;
}

/*
 * http_modify_header, modify HTTP header by flags
 */

char *http_modify_header(char *buf, int len, int flags) {	
	char *orig_hdr, *mod_hdr, *strtok_buf, *field;

	if (!is_http_header(buf, len)) {
		return NULL;
	}

	orig_hdr = strndup(buf, len);

	if (!orig_hdr) {
		lerror("calloc");
		return NULL;
	}

	mod_hdr = calloc(1, len * 2);

	if (!mod_hdr) {
		lerror("calloc");
		free(orig_hdr);
		return NULL;
	}

	strtok_buf = calloc(1, len + 1);

	if (!strtok_buf) {
		perror("calloc");
		free(mod_hdr);
		free(orig_hdr);
		return NULL;
	}

	field = strtok_r(orig_hdr, "\r\n", &strtok_buf);

	if (!field) {
		free(mod_hdr);
		free(orig_hdr);
		free(strtok_buf);
		return NULL;
	}

	strcat(mod_hdr, field);
	strcat(mod_hdr, "\r\n");

	field = strtok_r(NULL, "\r\n", &strtok_buf);

	while (field) {

		if (flags & HTTP_KILL_COOKIES) {
		
			if (strstr(field, "Cookie:") || strstr(field, "Set-Cookie:")) {
				field = strtok_r(NULL, "\r\n", &strtok_buf);
				continue;
			}
		}	

		if (flags & HTTP_FRENZY) {

			if (strstr(field, "Proxy-Connection:") || strstr(field, "Connection:") || strstr(field, "Keep-Alive")) {
				field = strtok_r(NULL, "\r\n", &strtok_buf);
				continue;
			}
		}

		strcat(mod_hdr, field);
		strcat(mod_hdr, "\r\n");

		field = strtok_r(NULL, "\r\n", &strtok_buf);
	}

	if (flags & HTTP_FRENZY) {
		if (!strstr(mod_hdr, "Host:") && !strstr(mod_hdr, "User-Agent:")) {
			strcat(mod_hdr, "Proxy-Connection: close\r\n");
			strcat(mod_hdr, "Connection: close\r\n");
		}
	}

	strcat(mod_hdr, "\r\n");

	return mod_hdr;	
}

/*
 * http_modify_chunk, modify HTTP chunk by given flags
 * * buf, given http chuk
 * * len, given http chunk length
 * * offset, new offset to buffer
 * * flags, given modify flags
 */

char *http_modify_chunk(char **buf, int len, int flags) {
	char *hdr, *r_buf;
	int hdr_length;

	r_buf = (*buf);

	if (!r_buf) {
		return NULL;
	}

	hdr_length = http_gethdrsize(r_buf);

	if (hdr_length < 8) {
		return NULL;
	}

	hdr = http_modify_header(r_buf, hdr_length, flags);

	if (!hdr) {
		return NULL;
	}

	(*buf) += hdr_length;

	return hdr;
}

/*
 * http_get_url, extract url from HTTP chunk
 * * buf, http chunk
 */

char *http_get_url(char *buf) {
	char *tmp, *url, *eou;
	int url_length;

	if (!buf) {
		return NULL;
	}

	tmp = strstr(buf, "http://");

	if (!tmp) {
		return NULL;
	}

	eou = strchr(tmp, ' ');
	
	if (!eou) {
		return NULL;
	}

	url_length = eou - tmp;
	
	if (url_length < 1) {
		return NULL;
	}

	url = calloc(1, url_length);

	if (!url) {
		return NULL;
	}

	memcpy(url, tmp, url_length);

	return url;
}
