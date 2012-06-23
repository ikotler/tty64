/*
 * postprocs.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <elf.h>

#include "misc.h"
#include "shbuf.h"
#include "shells.h"
#include "elf_ext.h"
#include "elf.h"
#include "crypt.h"

/*
 * gizmo_postproc_crypttxt, encrypt the .text section with given password
 * * src, given source ELF image
 * * dst, given destnation ELF image
 */

void *gizmo_postproc_crypttxt(void *src, void *dst) {
	char *passwd;
	gizmo_elf *image;
	Elf32_Shdr *txtsct;
	int fd, w_bytes, r_bytes;
	char *buf;

	image = (gizmo_elf *) dst;

	buf = calloc(1, PAGE_SIZE);

	if (!buf) {
		perror("calloc");
		return NULL;
	}

	passwd = passwdloop();

	if (!passwd) {
		return NULL;
	}
	
	txtsct = gizmo_elf_getsctbyname(image, ".text");

	if (!txtsct) {
		fprintf(stderr, "*** %s : Unable to locate .TEXT section, Rollback!\n", image->raw->filename);
		free(passwd);
		return NULL;
	}

	if (image->raw->fp) {
		fclose(image->raw->fp);
	}
	
	fd = open(image->raw->filename, O_RDWR);

	if (fd < 0) {
		perror(image->raw->filename);
		free(passwd);
		return NULL;
	}

	lseek(fd, txtsct->sh_offset, SEEK_SET);

	w_bytes = r_bytes = 0;

	while (w_bytes < txtsct->sh_size) {

		r_bytes = read(fd, buf, PAGE_SIZE);
		lseek(fd, (r_bytes * -1), SEEK_CUR);

		if ( (w_bytes + r_bytes) > txtsct->sh_size) {
			r_bytes -= (w_bytes + r_bytes) - txtsct->sh_size;
		}

		buf = crypt_xorbuf(buf, r_bytes, passwd);

		write(fd, buf, r_bytes);

		w_bytes += r_bytes;
	}

	close(fd);

	image->raw->fp = NULL;

	return image;
}

