/*
 * procfs_read.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

/*
 * procfs_pexename, print 'exe' link (original executable) of given pid proc entry
 * * pid, given entry pint
 */

void procfs_pexename(int pid) {
	char path[256], buf[1024];
		
	snprintf(path, sizeof(path), "/proc/%d/exe", pid);

	if (readlink(path, buf, sizeof(buf)) < 0) {
		fprintf(stderr, "*** %s: %s\n", path, strerror(errno));
		return ;
	}

	printf("* Executable: %s\n", buf);
	return ;
}
