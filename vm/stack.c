#include <stdio.h>
#include <string.h>
#include "vm.h"
#include "mem.h"
#include "ops.h"

int set_sp(struct proc *p, int addr)
{
	addr = word2int(p->r);
	p->stack_base = addr;
	memcpy(p->sp, p->r, 4);
	return 0;
}

int get_sp(struct proc *p, int addr)
{
	memcpy(p->r, p->sp, 4);
	return 0;
}

int push(struct proc *p, int addr)
{
	char *stackp;
	
	addr = word2int(p->sp);
	if (addr == -1) {
		fprintf(stderr, "push: Non-numeric stack pointer\n");
		return -1;
	}
	addr++;
	int2word(addr, p->sp);
	stackp = get_wordref(p, addr);
	if (stackp == NULL) {
		fprintf(stderr, "push: failed to get memory ref\n");
		return -1;
	}
	memcpy(stackp, p->r, 4);
	release_wordref(p, addr);
	return 0;
}

int pop(struct proc *p, int addr)
{
	char *stackp;
	
	addr = word2int(p->sp);
	if (addr <= p->stack_base) {
		fprintf(stderr, "pop: pop on empty stack\n");
		return 0;
	}
	stackp = get_wordref(p, addr);
	if (stackp == NULL) {
		fprintf(stderr, "pop: failed to get memory ref\n");
		return -1;
	}
	memcpy(p->r, stackp, 4);
	release_wordref(p, addr);
	addr--;
	int2word(addr, p->sp);
	return 0;
}
