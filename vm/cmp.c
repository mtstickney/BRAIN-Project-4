#include <stdio.h>
#include <string.h>
#include "vm.h"
#include "mem.h"
#include "ops.h"

int cmp_eql(struct proc *p, int addr)
{
	char *temp;

	temp = get_wordref(p, addr); 
	if (temp == NULL) {
		fprintf(stderr, "cmp_eql: failed to get memory ref\n");
		return -1;	
	}
	if (memcmp(p->r, temp, 4) == 0)
		p->c = 'T';
	else
		p->c = 'F';
	release_wordref(p, addr);
	return 0;
}

int cmp_less(struct proc *p, int addr)
{
	char *temp;

	temp = get_wordref(p, addr);
	if (temp == NULL) {
		fprintf(stderr, "cmp_less: failed to get memory ref\n");
		return -1;
	}
	if (memcmp(p->r, temp, 4) < 0)
		p->c = 'T';
	else
		p->c = 'F';
	release_wordref(p, addr);
	return 0;
}
