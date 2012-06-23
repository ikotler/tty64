/*
 * elf_ext.h
 */

#ifndef PAGE_SIZE
        #define PAGE_SIZE 4096
#endif

typedef struct gizmo_elf gizmo_elf;
typedef struct _g_raw gizmo_elf_raw;
typedef struct _g_hdrs gizmo_elf_hdrs;
typedef struct _g_attributes gizmo_elf_attrs;

struct gizmo_elf {

	struct _g_raw {
		FILE *fp;
		char *filename;
	} *raw;

	struct _g_hdrs {
		Elf32_Ehdr *ehdr;
		Elf32_Phdr *phdr;
		Elf32_Shdr *shdr;
		char *str_tbl;
	} *hdrs;

	struct _g_attributes {
		int padding_length;
		int gizmo_raw_off;
		int gizmo_vrt_off;
		char *gizmo_section;
	} *gizmo_attributes;

	gizmo_shcode *shcode;
};
