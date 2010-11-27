#include <stdio.h>
#include "vm.h"
#include "mem.h"
#include "math.h"
#include "ops.h"

static int stack_op(struct proc *p, int addr, char op, char *opname)
{
	char word1[4];
	char word2[4];
	int res;
	
	addr = word2int(p->sp);
	if (addr < p->stack_base+1) {
		fprintf(stderr, "%s: stack contains fewer than 2 elements\n", opname);
		return -1;
	}
	
	if (load(p, addr--, word1) == -1 || load(p, addr, word2) == -1) {
		fprintf(stderr, "%s: load failed\n", opname);
		return -1;
	}
	/* order is next_on_stack op top_of_stack */
	res = do_binop(word2, word1, op);
	if (res < 0) {
		fprintf(stderr, "%s: do_binop failed\n", opname);
		return -1;
	}
	int2word(res, word1);
	int2word(addr, p->sp);
	if (store(p, word1, addr) == -1) {
		fprintf(stderr, "%s: store failed\n", opname);
		return -1;
	}
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
