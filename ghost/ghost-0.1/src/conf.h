/*
 * conf.h
 */

enum cfg_validation_modes {
	ERROR_MODE,
	GLITCH_MODE
};

hashtable *cfg_parse(char *);

int cfg_validate(int, char *);
int cfg_validate_all(hashtable *, int mode);

int cfg_get_proxyopts(void);
int cfg_get_clientopts(void);
char *cfg_get_filename(void);

void build_cfg_bitmask_opts(hashtable *);

int config_dump(FILE *);
