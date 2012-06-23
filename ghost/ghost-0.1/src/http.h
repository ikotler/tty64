/*
 * http.h
 */

char *http_get_url(char *);
int http_gethdrsize(char *);

char *http_modify_chunk(char **, int, int);
