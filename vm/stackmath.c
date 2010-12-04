#include <stdio.h>
#include "vm.h"
#include "mem.h"
#include "math.h"
#include "ops.h"

static int stack_op(struct proc *p, int addr, char op, char *opname)
{
	char *word1;
	char *word2;
	int res;
	
	addr = word2int(p->sp);
	if (addr < p->stack_base+1) {
		fprintf(stderr, "%s: stack contains fewer than 2 elements\n", opname);
		return -1;
	}
	
	word1 = get_wordref(p, addr);
	word2 = get_wordref(p, addr-1);
	if (word1 == NULL || word2 == NULL) {
		fprintf(stderr, "%s: failed to get memory ref\n", opname);
		return -1;
	}
	/* order is next_on_stack op top_of_stack */
	res = do_binop(word2, word1, op);
	release_wordref(p, addr);
	release_wordref(p, addr-1);
	if (res < 0) {
		fprintf(stderr, "%s: do_binop failed\n", opname);
		return -1;
	}
	addr--;
	int2word(addr, p->sp);
	word1 = get_wordref(p, addr);
	if (word1 == NULL) {
		fprintf(stderr, "%s: failed to get memory ref\n", opname);
		return -1;
	}
	int2word(res, word1);
	release_wordref(p, addr);
	return 0;
}

int add_stack(struct proc *p, int addr)
{
	return stack_op(p, addr, '+', "add_stack");
}

int subtract_stack(struct proc *p, int addr)
{
	return stack_op(p, addr, '-', "add_stack");
}

int multiply_stack(struct proc *p, int addr)
{
	return stack_op(p, addr, '*', "multiply_stack");
}

int divide_stack(struct proc *p, int addr)
{
	return stack_op(p, addr, '/', "divide_stack");
}
