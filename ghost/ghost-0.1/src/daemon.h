/*
 * daemon.h 
 */

void daemon_start(void);

void *daemon_to_proxy(char **, int);
void *daemon_to_client(char **, int);
