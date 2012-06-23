/*
 * proxy.h
 */

int proxy_parse(void);

int proxy_getrandproxy(void);
void proxy_rebuild_db(void);
void proxy_build_limits(void);
int proxy_get_iotimeout(void);

int proxy_delete(int);
int proxy_add(char *, int);
int proxy_mod(int, char *, int);
int proxy_dump(FILE *);

int proxy_validateall(void);
int proxy_t_validate(int);
int proxy_t_validate_http(int, char **);
