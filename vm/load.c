#include <stdio.h>
#include <string.h>
#include "vm.h"
#include "mem.h"
#include "ops.h"

int load_all(struct proc *p, int addr)
{
	if (load(p, addr, p->r) == -1) {
		fprintf(stderr, "load_all: load failed\n");
		return -1;
	}
	return 0;
}

int load_low(struct proc *p, int addr)
{
	char temp[4];

	if (load(p, addr, temp) == -1) {
		fprintf(stderr, "load_low: load failed\n");
		return -1;
	}
	memcpy(p->r+2, temp+2, 2);
	return 0;
}

int load_high(struct proc *p, int addr)
{
	char temp[4];

	if (load(p, addr, temp) == -1) {
		fprintf(stderr, "load_high: load failed\n");
		return -1;
	}
	memcpy(p->r, temp, 2);
	return 0;
}
