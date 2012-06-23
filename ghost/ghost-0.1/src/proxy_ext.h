/*
 * proxy_ext.h
 */

#ifndef _MAX_PROXIES
	#define _MAX_PROXIES 655356
#endif

#define VALIDATE_SOCKET    0x01
#define VALIDATE_CONTENT   0x10

typedef struct proxy_entry proxy_entry;
typedef struct proxy_db proxy_db;
typedef struct proxy_db_stats proxy_db_stats;

enum proxy_state {
	WORKING,
	DEAD,
	BROKEN,
	UNKNOWN
};

struct proxy_entry {
	char *addr;
	int port;
	int strikes;
	int jobs;
	int state;
};

struct proxy_db_stats {
	int index;
	int valids;
	int deads;
	int unknowns;
	int broken;
	int holes;
};

struct proxy_db {
	proxy_entry **db;
	proxy_db_stats stats;
};

char *proxy_getrank(proxy_entry *);
proxy_entry *proxy_querydb(int);
proxy_db_stats *proxy_get_stats(void);

