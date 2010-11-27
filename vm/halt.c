#include <stdio.h>
#include "vm.h"
#include "mem.h"
#include "sched.h"
#include "ops.h"

int halt(struct proc *p, int addr)
{
	int base, limit;

	base = word2int(p->br);
	limit=word2int(p->lr);
	if (base < 0 || limit < 0) {
		fprintf(stderr, "halt: bad base or limit register, not freeing memory\n");
	} else {
		freemem(base, limit+1);
	}
	sched_reset(p->pid);
	sched_suspend(p->pid);
	return 0;
}
