/*
 * preprocs.c
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <elf.h>

#include "shbuf.h"
#include "shells.h"
#include "elf_ext.h"
#include "elf.h"
#include "x86.h"
#include "preprocs.h"

/*
 * gizmo_preproc_simple, simple and clean springboard to the real entry point
 * * src, given source image
 * * dst, given dest image
 */

shbuf *gizmo_preproc_simple(void *src, void *dst) {
	gizmo_elf *image = (gizmo_elf *) src;
	shbuf *buf;

	buf = shbuf_create(PREPROC_SIMPLE_LEN, '\x90');

	if (!buf) {
		return NULL;
	}

	shbuf_append(buf, X86_ASSEMBLY_MOV_EBP, (sizeof(X86_ASSEMBLY_MOV_EBP)-1));
	shbuf_append(buf, (char *) &image->hdrs->ehdr->e_entry, sizeof(Elf32_Addr));
	shbuf_append(buf, X86_ASSEMBLY_JMP_EBP, (sizeof(X86_ASSEMBLY_JMP_EBP)-1));

	/*
	 * \xBD\<ENTRY_POINT>\xFF\xE5
	 */

	return buf;
}

/*
 * gizmo_preproc_crypttxt, stack up the .text section address and size and creates a springboard
 * * src, given source image
 * * dst, given dest image
 */

shbuf *gizmo_preproc_crypttxt(void *src, void *dst) {
	gizmo_elf *image = (gizmo_elf *) src;
	Elf32_Shdr *txtsct;
	shbuf *buf, *ptr;
	int idx;

	buf = shbuf_create(PREPROC_CRYPTTXT_LEN, '\x90');

	if (!buf) {
		return NULL;
	}

	/*
	 * \xFC\xBE
	 */

	shbuf_append(buf, X86_ASSEMBLY_CLD, (sizeof(X86_ASSEMBLY_CLD)-1));
	shbuf_append(buf, X86_ASSEMBLY_MOV_ESI, (sizeof(X86_ASSEMBLY_MOV_ESI)-1));

	ptr = gizmo_preproc_simple(src, dst);

	if (!ptr || !ptr->buf) {

		if (ptr) {
			shbuf_free(ptr);
		}

		shbuf_free(buf);
		return NULL;
	}

	/*
	 * One should never never assume that e_entry point's to .text section, This could create collision between two or more gizmos.
	 * Therefor the above code seeking for the section and places the address and size differently from e_entry
	 */

	txtsct = gizmo_elf_getsctbyname(image, ".text");

	if (!txtsct) {
		fprintf(stderr, "*** %s : Unable to locate .TEXT section, Rollback!\n", image->raw->filename);
		shbuf_free(buf);
		return NULL;
	}

	/*
	 * <.TEXT ADDRESS>\x89\xF7\xB9\<.TEXT LENGTH>\xC3\<SPRINGBOARD>
	 */

	shbuf_append(buf, (char *) &txtsct->sh_addr, sizeof(Elf32_Addr));
	shbuf_append(buf, X86_ASSEMBLY_MOV_ESI_EDI, (sizeof(X86_ASSEMBLY_MOV_ESI_EDI)-1));
	shbuf_append(buf, X86_ASSEMBLY_MOV_ECX, (sizeof(X86_ASSEMBLY_MOV_ECX)-1));
	shbuf_append(buf, (char *) &txtsct->sh_size, sizeof(Elf32_Word));
	shbuf_append(buf, X86_ASSEMBLY_RET, (sizeof(X86_ASSEMBLY_RET)-1));
	shbuf_append(buf, ptr->buf, PREPROC_SIMPLE_LEN);

	/*
	 * This is better then mprotect()
	 */

	if (!(txtsct->sh_flags & SHF_WRITE)) {

		image = (gizmo_elf *) dst;

		txtsct = gizmo_elf_getsctbyname(image, ".text");
		txtsct->sh_flags |= SHF_WRITE;

	       	for (idx = 0; idx < image->hdrs->ehdr->e_phnum; idx++) {
	                if (image->hdrs->phdr[idx].p_type == PT_LOAD && image->hdrs->phdr[idx].p_offset == 0) {
				image->hdrs->phdr[idx].p_flags |= PF_W;
			}
		}
	}

	shbuf_free(ptr);

	/*
	 * \xFC\xBE\<.TEXT ADDRESS>\x89\xF7\xB9\<.TEXT LENGTH>\xC3\<SPRINGBOARD>
	 */	

	return buf;
}
