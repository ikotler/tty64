/*
 * process.h
 */

process *process_attach(int);
process *process_create(char *, char **, char **);

process *process_getcur(void);
void process_setcur(process *);

char **process_argv_maker(int, char **, int);
void process_argv_free(char **);

int process_start(process *, opts *);
void process_destory(process *);
