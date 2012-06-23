/*
 * process.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/ptrace.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>

#include <linux/user.h>

#include "gfx.h"
#include "hexdump.h"
#include "io.h"
#include "utilities.h"
#include "opts_ext.h"
#include "process_ext.h"
#include "procfs.h"

#include "consts.h"

extern int verbose;

static process *cur_proc;

/*
 * _perror_int, perror() hack to accept int instead of string as reason
 * * num, given integer
 */

__inline__ void _perror_int(int num) {
	fprintf(stderr, "%d: %s\n", num, strerror(errno));
	return ;
}

/*
 * process_argv_free, free argv vector
 * * argv, given argv
 */

void process_argv_free(char **argv) {
	int idx;

	for (idx = 0; argv[idx] != NULL; idx++) {
		free(argv[idx]);
	}

	return ;
}

/*
 * process_vec_len, return vector length 
 * vec, given vector
 */

int process_vlen(void **vec) {
	int j, len;

	len = 0;

	for (j = 0; vec[j] != NULL; j++) {
		len += strlen(vec[j]);
	}

	return len;
}

/*
 * process_destory, clean up a process entry
 * proc, given process entry
 */

void process_destory(process *proc) {

	if (!proc) {
		return ;
	}

	if (proc->alive) {

		if (ptrace( (proc->kill_it) ? PTRACE_KILL : PTRACE_DETACH, proc->pid, NULL, NULL) < 0) {
			_perror_int(proc->pid);
		}
	}

	if (proc->vectors) {
		process_argv_free(proc->vectors->argv);
		free(proc->vectors);
	}

	free(proc);

	return ;
}

/*
 * process_waitup, suspend until child stopped or terminated
 */

int process_waitup(void) {
	int status, rval;

	rval = wait(&status);

	if (rval < 0) {
		perror("wait");
		return -1;
	}

	if (WIFEXITED(status) || WIFSIGNALED(status)) {
		fprintf(stderr, "*** (%d): child been unexpectedly terminated! ***", rval);
		return -1;
	}

	if (WIFSTOPPED(status)) {
		return 1;
	}
	
	/* NEVER REACH */
	return -1;
}

/*
 * process_attach, create process via pid attachment
 * * pid, given pid
 */

process *process_attach(int pid) {
	process *proc;

	if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) < 0) {
		_perror_int(pid);
		return NULL;
	}

	proc = calloc(1, sizeof(process));

	if (!proc) {

		perror("calloc");

		if (ptrace(PTRACE_DETACH, pid, NULL, NULL) < 0) {
			_perror_int(pid);
		}

		free(proc);
		return NULL;
	}

	proc->pid = pid;

	if (process_waitup() < 0) {
		free(proc);
		return NULL;
	}

	if (verbose) {
		gfx_box("PID Attachment #%d", proc->pid);
		fputc('\t', stdout);
		procfs_pexename(proc->pid);
		fputc('\n', stdout);
	}

	proc->alive = 1;

	return proc;
}

/*
 * process_create, create process via execuve()
 * * filename, given application filename
 * * argv, given argv array
 * * envp, given enviroment array
 */

process *process_create(char *filename, char **argv, char **envp) {
	process *proc;

	proc = calloc(1, sizeof(process));

	if (!proc || !argv) {

		if (!proc) {
			perror("calloc");
		}

		return NULL;
	}

	if (access(filename, X_OK)) {
		free(proc);
		perror(filename);
		return NULL;
	}

	proc->vectors = calloc(1, sizeof(struct process_vectors));

	if (!proc->vectors) {
		free(proc);
		perror("calloc");
	}
	
	proc->vectors->argv = argv;
	proc->vectors->envp = envp;

	proc->pid = fork();
	proc->kill_it = 1;

	if (proc->pid < 0) {
		process_destory(proc);
		perror("fork");
		return NULL;
	}

	if (proc->pid == 0) {
	
		if (ptrace(PTRACE_TRACEME, 0, 0, 0) < 0) {
			perror(proc->vectors->argv[0]);
			kill(getpid(), SIGKILL);
			_exit(-1);
		}

		_exit(execve(proc->vectors->argv[0], proc->vectors->argv, proc->vectors->envp));
	}

	if (process_waitup() < 0) {
		process_destory(proc);
		return NULL;
	}

	if (verbose) {
		gfx_box("Executing %s", proc->vectors->argv[0]);
		printf("\t* Envp Size: %d bytes\n", process_vlen((void *)proc->vectors->envp));
		printf("\t* Argv Size: %d bytes\n", process_vlen((void *)proc->vectors->argv));
		fputc('\n', stdout);
	}

	proc->alive = 1;

	return proc;
}

/*
 * process_start, the processor
 * * proc, given process
 * * optz, given options
 */

int process_start(process *proc, opts *optz) {
	long data;
	int j, pfaults, un_data, u_data;
	void *head_addr, *tail_addr;

	head_addr = tail_addr = NULL;
	pfaults = un_data = u_data =0;

	switch (optz->action) {

		case PROCESS_DUMP:

			if (hexdump_initialization(optz->bir, optz->s_addr, optz->e_addr) < 0) {
				return 0;
			}

			gfx_box("Memory Dump [0x%08x - 0x%08x] <%lu bytes>", optz->s_addr, optz->e_addr, addr_substruct(optz->s_addr, optz->e_addr));

			while (addr_cmp(optz->s_addr,optz->e_addr)) {

				data = ptrace(PTRACE_PEEKTEXT, proc->pid, optz->s_addr, NULL);

				if (data < 0 && (errno == EFAULT || errno == EIO)) {

					pfaults++;
					un_data+=PTRACE_WORD;

				} else {

					u_data+=PTRACE_WORD;
					io_out_write(data);

				}

				hexdump_push(data);
				hexdump_display();

				if (optz->bir < PTRACE_WORD) {

					for (j = 5 - optz->bir; j > 0; j--) {
						hexdump_display();
					}
				}

        			optz->s_addr = addr_increase(optz->s_addr, PTRACE_WORD);
			}

			hexdump_finialization();
			break;

		case PROCESS_SCAN:

			gfx_box("Memory Mapping [0x%08x - 0x%08x] <%lu bytes>", optz->s_addr, optz->e_addr, addr_substruct(optz->s_addr, optz->e_addr));

			while (addr_cmp(optz->s_addr, optz->e_addr)) {

				data = ptrace(PTRACE_PEEKTEXT, proc->pid, optz->s_addr, NULL);

				if (data < 0 && (errno == EFAULT || errno == EIO)) {

					pfaults++;
					un_data += PTRACE_WORD;	

					if (head_addr) {

						if (head_addr == tail_addr) {
							head_addr = addr_decrease(head_addr, PTRACE_WORD);
						}

						printf("\t * %p - %p <%lu bytes>\n", head_addr, tail_addr, addr_substruct(head_addr, tail_addr));

						head_addr = tail_addr = NULL;
					}

				} else {

					u_data += PTRACE_WORD;

					if (!head_addr) {
						head_addr = optz->s_addr;
					}

					tail_addr = optz->s_addr;
				}

				optz->s_addr = addr_increase(optz->s_addr, PTRACE_WORD);
			}

			if (head_addr) {
				printf("\t * %p - %p+ <%lu bytes> \n", head_addr, tail_addr, addr_substruct(head_addr, tail_addr));
			}

			break;

		case PROCESS_PATCH:

			optz->e_addr = optz->s_addr + io_in_totbytes();

			gfx_box("Memory Patching [0x%08x - 0x%08x] <%lu bytes>", optz->s_addr, optz->e_addr, addr_substruct(optz->s_addr, optz->e_addr));

			while (addr_cmp(optz->s_addr, optz->e_addr)) {

				printf("\t (%p) : ", optz->s_addr);

				data = ptrace(PTRACE_PEEKTEXT, proc->pid, optz->s_addr, NULL);

				if (data < 0 && (errno == EFAULT || errno == EIO)) {
					printf("<ERR: %s>\n", strerror(errno));
					continue;
				}

				head_addr = (void *) data;
				tail_addr = (void *) io_in_read();

				data = ptrace(PTRACE_POKETEXT, proc->pid, optz->s_addr, tail_addr);

                                if (data < 0 && (errno == EFAULT || errno == EIO)) {
                                        printf("<ERR: %s>\n", strerror(errno));
                                        continue;
                                }

				printf("<%x> [orig: %x] | ", (unsigned int) tail_addr, (unsigned int) head_addr);
		
				data = ptrace(PTRACE_PEEKTEXT, proc->pid, optz->s_addr, NULL);

				if (data < 0 && (errno == EFAULT || errno == EIO)) {
					printf("<ERR: %s>\n", strerror(errno));
					continue;
				}

				printf("(%x)\n", (unsigned int) data);

				optz->s_addr = addr_increase(optz->s_addr, PTRACE_WORD);
			}			
			
			break;
	}

	fputc('\n', stdout);
		
	if (verbose && (optz->action == PROCESS_DUMP || optz->action == PROCESS_SCAN)) {
		gfx_box("Memory Summary");
		printf("\t * Total Pagefaults: %d time(s)\n", pfaults);
		printf("\t * Total Unused Memory: %d bytes\n", un_data);
		printf("\t * Total Used Memory: %d bytes\n\n", u_data);
	}

	return 1;
}

/*
 * process_argv_maker, create argv vector from given argv from given index
 * * idx, given index in exists argv
 * * argv, given exists argv
 * * argc, given exists argc
 */

char **process_argv_maker(int idx, char **argv, int argc) {
	char **buf;
	int j = 0;

	buf = calloc(argc - idx, sizeof(char*));

	if (!buf) {
		perror("calloc");
		return NULL;
	}

	for (; idx < argc; idx++) {

		buf[j] = calloc(1, strlen(argv[idx]));
		
		if (!buf[j]) {

			for (j--; j > 0; j--) {
				free(buf[j]);								
			}

			free(buf);
			perror("calloc");
			return NULL;
		}
	
		strncpy(buf[j], argv[idx], strlen(argv[idx]));
		j++;
	}	

	return buf;
}

/*
 * process_setcur, set current process context
 * * proc, given current application
 */

void process_setcur(process *proc) { 
	cur_proc = proc;
}

/*
 * process_getcur, return current process context
 */

process *process_getcur(void) { 
	return cur_proc; 
}
