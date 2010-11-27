#include <stdio.h>
#include "vm.h"
#include "mem.h"
#include "ops.h"

int load_shared(struct proc *p, int addr)
{
	if (load_shm(addr, p->r) != 0) {
		fprintf(stderr, "load_shared: load failed (pid %u)\n", p->pid);
		return 1;
	}
	return 0;
}

int store_shared(struct proc *p, int addr)
{
	if (store_shm(p->r, addr) != 0) {
		fprintf(stderr, "store_shared: store failed (pid %u)\n", p->pid);
		return 1;
	}
	return 0;
}
