/*
 * elf.h
 */

int gizmo_elf_processor(char *, char *, gizmo_shcode *);
Elf32_Shdr *gizmo_elf_getsctbyname(gizmo_elf *, char *);

