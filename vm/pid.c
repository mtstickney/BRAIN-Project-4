#include "vm.h"
#include "ops.h"

int identify(struct proc *p, int addr)
{
	int2word(p->pid, p->r);
	return 0;
}
