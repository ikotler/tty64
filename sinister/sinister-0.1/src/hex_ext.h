/*
 * hex_ext.h
 */

typedef struct hex_entry hex_entry;
typedef struct hex_stack hex_stack;

struct hex_entry {
	char data;
	hex_entry *next;
};

struct hex_stack {
	hex_entry *head;
	hex_entry *tail;
	int totbytes;
	int bir;
	void *baddr;
	void *eaddr;
};
