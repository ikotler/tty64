/*
 * log.h
 */

int log_open(void);
void log_close(void);

void lprintf(char *, ...);
void lerror(char *);
void lraw(char *);

int log_restart(void);
