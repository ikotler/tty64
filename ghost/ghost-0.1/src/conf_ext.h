/*
 * conf_ext.h
 */

#ifndef _MAX_CONF_OPTS
	#define _MAX_CONF_OPTS 17
#endif

typedef struct cfg_validate_entry cfg_validate_entry;
typedef struct cfg_bitmask_opt cfg_bitmask_opt;
typedef struct cfg_validate_cb cfg_validate_cb;

struct cfg_validate_entry {
	char *tag;
	int tag_type;
	int tag_status;
	char *dep_tag;
	int tag_mode;
};

struct cfg_bitmask_opt {
	char *opt_name;
	int bit_value;
	int side_effected;
};

struct cfg_validate_cb {
	int tag_type;
	char *type_name;
	int (*validate)(char *, int);
};

enum cfg_entry_type {
	IS_STRING = 0,
	IS_NUMERIC,
	IS_BOOLEAN,
	IS_URL
};

enum cfg_tag_status {
	OPTINIAL = 0,
	MANDATORY
};

enum cfg_flipflop_modes {
	STATIC = 0,
	DYANMIC	
};
