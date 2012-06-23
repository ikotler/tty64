/*
 * shells.h
 */

typedef struct gizmo_shcode gizmo_shcode;

struct gizmo_shcode {
	char *arch;
	char *platform;
	char *desc;
	int sh_len;
	char *sh;
	int data_len;
	shbuf *(*gizmo_preproc) (void *, void *);
	void *(*gizmo_postproc) (void *, void *);
};

int shells_init(void);

void shells_shrtdesc(int, gizmo_shcode);
void shells_fulldesc(int, gizmo_shcode);

void shells_foreach(void (*cb)(int, gizmo_shcode));

gizmo_shcode *shells_get(int);

