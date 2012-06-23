/*
 * io.h
 */

void io_release(void);
void io_killbufs(void);
void io_fastrelease(void);
void io_buffer(void);

int io_foutput(int, char *);

char *io_get_stdout(void);
char *io_get_stderr(void);

