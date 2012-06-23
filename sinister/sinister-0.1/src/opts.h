/*
 * opts.h
 */

opts *opts_alloc_default(void);

void opts_err_actconflict(void);
void opts_err_wrongopt(char *, char *);
void opts_err_badaddr(char *);
void opts_free(opts *);

int opts_has_direction(opts *);
int opts_has_action(opts *);
int opts_has_cmdparams(opts *);
