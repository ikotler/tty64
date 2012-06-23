/*
 * lib_ext.h
 */

typedef struct libptr libptr;
	
struct __libptr_stack {
	int stack_items;
	unsigned long **stack;
};

struct libptr {
	char *lib_name;
	char *lib_path;
	char *fcn_name;
	void *(*fcn_handler)();
	void *lib_handler;
	unsigned long fcn_rval;
	struct __libptr_stack *stack;
};
