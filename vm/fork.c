#include <stdio.h>
#include <string.h>
#include "vm.h"
#include "sched.h"
#include "mem.h"
#include "ops.h"

int fork_proc(struct proc *p, int addr)
{
	struct proc *newp;
	int procsize;

	procsize = word2int(p->lr);
	if (procsize < 0) {
		fprintf(stderr, "fork_proc: bad limit register (pid %u)\n", p->pid);
		return 1;
	}
	newp = procalloc(procsize);
	if (newp == NULL) {
		/* parent gets error value on failure */
		memset(p->r, '0', 4);
	} else {
		int2word(newp->pid, p->r);
		int2word(p->pid, newp->r);
	}
	memcpy(newp->ic, p->ic, 2);
	newp->c = p->c;
	newp->stack_base = p->stack_base;

	if (dup_mem(p, newp) != 0) {
		fprintf(stderr, "fork_proc: dup_mem failed\n");
		return -1;
	}

	sched_reset(newp->pid);
	sched_resume(newp->pid);
	return 0;
}
