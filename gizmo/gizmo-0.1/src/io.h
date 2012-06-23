/*
 * io.h
 */

void *io_getchunk(FILE *, long, int);
int io_chunkcpy(FILE *, FILE *, long, int, int);
int io_filesize(FILE *);

