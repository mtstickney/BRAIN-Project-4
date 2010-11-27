#include <string.h>
#include "vm.h"
#include "ops.h"

int jmp(struct proc *p, int ic)
{
	char temp[4];

	int2word(ic, temp);
	memcpy(p->ic, temp+2, 2);
	return 0;
}

int jmp_if(struct proc *p, int addr)
{
	if (p->c == 'T') {
		jmp(p, addr);
	}
	return 0;
}
