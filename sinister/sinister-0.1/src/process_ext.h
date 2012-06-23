/*
 * process_ext.h
 */

typedef struct process process;
typedef struct process_vectors process_vectors;

struct process_vectors {
	char **argv;
	char **envp;
};

struct process {
	pid_t pid;
	int kill_it;
	int alive;
	process_vectors *vectors;
};

enum process_actions {
	PROCESS_NOP = 0,
	PROCESS_DUMP,
	PROCESS_PATCH,
	PROCESS_SCAN
};
