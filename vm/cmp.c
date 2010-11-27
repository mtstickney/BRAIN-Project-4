#include <stdio.h>
#include <string.h>
#include "vm.h"
#include "mem.h"
#include "ops.h"

int cmp_eql(struct proc *p, int addr)
{
	char temp[4];

	if (load(p, addr, temp) == -1) {
		fprintf(stderr, "cmp_eql: load failed\n");
		return -1;	
	}
	if (memcmp(p->r, temp, 4) == 0)
		p->c = 'T';
	else
		p->c = 'F';
	return 0;
}

int cmp_less(struct proc *p, int addr)
{
	char temp[4];

	if (load(p, addr, temp) == -1) {
		fprintf(stderr, "cmp_less: load failed\n");
		return -1;
	}
	if (memcmp(p->r, temp, 4) < 0)
		p->c = 'T';
	else
		p->c = 'F';
	return 0;
}