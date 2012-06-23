/*
 * opts.h
 */

typedef struct sinister_opts opts;
 
struct sinister_opts {
	int bir;
	int direction;
	int action;
	int cmdparams;
	void *s_addr;
	void *e_addr;
};
