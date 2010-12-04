#include <stdio.h>
#include <string.h>
#include "vm.h"
#include "mem.h"
#include "ops.h"

int store_register(struct proc *p, int addr)
{
	char *temp;
	
	temp = get_wordref(p, addr);
	if (temp == NULL) {
		fprintf(stderr, "store_register: failed to get memory ref\n");
		return -1;
	}
	memcpy(temp, p->r, 4);
	release_wordref(p, addr);
	return 0;
}
