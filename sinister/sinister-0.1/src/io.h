/*
 * io.h 
 */

void io_init(void);
void io_fini(void);
int io_has_in(void);

int io_out_init(char *);
int io_in_init(char *);

int io_out_write(long);
int io_in_totbytes(void);
long io_in_read(void);

