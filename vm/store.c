#include <stdio.h>
#include "vm.h"
#include "mem.h"
#include "ops.h"

int store_register(struct proc *p, int addr)
{
	if (store(p, p->r, addr) == -1) {
		fprintf(stderr, "store_register: store failed\n");
		return -1;
	}
	return 0;
}