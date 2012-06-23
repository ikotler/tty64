/* 
 * hydrogen.c (ik@ikotler.org)
 *	\ binary executable handler
 *
 * This is basically a 'hacked' version of (/usr/src/linux/fs)/binfmt_aout.c
 */

#include <linux/module.h>

#include <linux/time.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/a.out.h>
#include <linux/mman.h>
#include <linux/errno.h>
#include <linux/signal.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/stat.h>
#include <linux/fcntl.h>
#include <linux/ptrace.h>
#include <linux/user.h>
#include <linux/slab.h>
#include <linux/binfmts.h>
#include <linux/personality.h>
#include <linux/init.h>

#include <asm/system.h>
#include <asm/uaccess.h>
#include <asm/cacheflush.h>

#define BAD_ADDR(x)     ((unsigned long)(x) >= TASK_SIZE)

static int load_bin(struct linux_binprm *, struct pt_regs *);

static struct linux_binfmt bin_format = {
	.module		= THIS_MODULE,
	.load_binary	= load_bin
};

static int set_brk(unsigned long start, unsigned long end)
{
        start = PAGE_ALIGN(start);
        end = PAGE_ALIGN(end);
        if (end > start) {
                unsigned long addr;
                down_write(&current->mm->mmap_sem);
                addr = do_brk(start, end - start);
                up_write(&current->mm->mmap_sem);
                if (BAD_ADDR(addr))
                        return addr;
        }
        return 0;
}

static unsigned long __user *create_bin_tables(char __user *p, struct linux_binprm * bprm)
{
        char __user * __user *argv;
        char __user * __user *envp;
        unsigned long __user *sp;
        int argc = bprm->argc;
        int envc = bprm->envc;

        sp = (void __user *)((-(unsigned long)sizeof(char *)) & (unsigned long) p);
#ifdef __sparc__
        /* This imposes the proper stack alignment for a new process. */
        sp = (void __user *) (((unsigned long) sp) & ~7);
        if ((envc+argc+3)&1) --sp;
#endif
#ifdef __alpha__
/* whee.. test-programs are so much fun. */
        put_user(0, --sp);
        put_user(0, --sp);
        if (bprm->loader) {
                put_user(0, --sp);
                put_user(0x3eb, --sp);
                put_user(bprm->loader, --sp);
                put_user(0x3ea, --sp);
        }
        put_user(bprm->exec, --sp);
        put_user(0x3e9, --sp);
#endif
        sp -= envc+1;
        envp = (char __user * __user *) sp;
        sp -= argc+1;
        argv = (char __user * __user *) sp;
#if defined(__i386__) || defined(__mc68000__) || defined(__arm__) || defined(__arch_um__)
        put_user((unsigned long) envp,--sp);
        put_user((unsigned long) argv,--sp);
#endif
        put_user(argc,--sp);
        current->mm->arg_start = (unsigned long) p;
        while (argc-->0) {
                char c;
                put_user(p,argv++);
                do {
                        get_user(c,p++);
                } while (c);
        }
        put_user(NULL,argv);
        current->mm->arg_end = current->mm->env_start = (unsigned long) p;
        while (envc-->0) {
                char c;
                put_user(p,envp++);
                do {
                        get_user(c,p++);
                } while (c);
        }
        put_user(NULL,envp);
        current->mm->env_end = (unsigned long) p;
        return sp;
}

static int load_bin(struct linux_binprm * bprm, struct pt_regs * regs) {
	char *fext;
	int fsize, error;
	unsigned long bin_entry;
	loff_t pos;

	/* 
	 * Checking for non-zero filesize and '.bin' extension
 	 */

	if ( (!(fsize = i_size_read(bprm->file->f_dentry->d_inode))) ||
	     (!(fext = strstr(bprm->filename, ".bin"))) || (strcmp(fext, ".bin")) )
	{
		return -ENOEXEC;
	}

	bin_entry = 0x00000000;
	pos = 0;

        error = flush_old_exec(bprm);
        if (error)
                return error;

	/* OK, This is the point of no return */	
        current->mm->end_code = fsize +
		(current->mm->start_code = bin_entry);

	current->mm->start_data = 0;
        current->mm->end_data = 0;
	current->mm->start_brk = 0;
        current->mm->brk = 0;
        current->mm->free_area_cache = current->mm->mmap_base;

	set_personality(PER_LINUX);

        set_mm_counter(current->mm, rss, 0);
        current->mm->mmap = NULL;
        compute_creds(bprm);
        current->flags &= ~PF_FORKNOEXEC;

        down_write(&current->mm->mmap_sem);
        error = do_brk(bin_entry & PAGE_MASK, fsize);
        up_write(&current->mm->mmap_sem);

        if (error != (bin_entry & PAGE_MASK))
		goto harakiri;

        error = bprm->file->f_op->read(bprm->file, (char __user *)bin_entry, fsize, &pos);
        if ((signed long)error < 0)
		goto harakiri;

        flush_icache_range(0, fsize);

        set_binfmt(&bin_format);

        error = set_brk(current->mm->start_brk, current->mm->brk);
        if (error < 0)
		goto harakiri;

        error = setup_arg_pages(bprm, STACK_TOP, EXSTACK_DEFAULT);
        if (error < 0)
		goto harakiri;

	current->mm->start_stack =
                (unsigned long) create_bin_tables((char __user *) bprm->p, bprm);

	/*
	 * There goes the neighborhood ... 
	 */

        start_thread(regs, bin_entry, current->mm->start_stack);

	if (unlikely(current->ptrace & PT_PTRACED)) {
                if (current->ptrace & PT_TRACE_EXEC)
                        ptrace_notify ((PTRACE_EVENT_EXEC << 8) | SIGTRAP);
                else
                        send_sig(SIGTRAP, current, 0);
        }

        return 0;

	harakiri:
		send_sig(SIGKILL, current, 0);
		return error;
}

int init_module(void) 
{
	return register_binfmt(&bin_format);
}

void cleanup_module(void) 
{
	unregister_binfmt(&bin_format);
	return ;
}

MODULE_LICENSE("GPL");
