/*
 * preprocs.h
 */

#ifndef PREPROC_SIMPLE_LEN

        #define PREPROC_SIMPLE_LEN \
		(sizeof(X86_ASSEMBLY_MOV_EBP)-1) + \
			(sizeof(Elf32_Addr)) + \
				(sizeof(X86_ASSEMBLY_JMP_EBP)-1)

#endif

shbuf *gizmo_preproc_simple(void *, void *);

#ifndef PREPROC_CRYPTTXT_LEN
	
	#define PREPROC_CRYPTTXT_LEN \
		(sizeof(X86_ASSEMBLY_CLD)-1) + (sizeof(X86_ASSEMBLY_MOV_ESI)-1) + (sizeof(Elf32_Addr)) + \
			(sizeof(X86_ASSEMBLY_MOV_ESI_EDI)-1) + (sizeof(Elf32_Word)) + (sizeof(X86_ASSEMBLY_RET)-1) + \
				PREPROC_SIMPLE_LEN + 1

#endif

shbuf *gizmo_preproc_crypttxt(void *, void *);

