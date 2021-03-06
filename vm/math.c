#include <stdio.h>
#include "vm.h"
#include "mem.h"
#include "ops.h"
#include "math.h"

int do_binop(char *a, char *b, char op)
{
	int i,j;

	i = word2int(a);
	j = word2int(b);
	if (i == -1 || j == -1) {
		fprintf(stderr, "do_binop: non-numeric operand\n");
		fprintf(stderr, "word 1: "); print_word(stderr, a);
		fprintf(stderr, " word 2: "); print_word(stderr, b);
		fprintf(stderr, "\n");
		return -1;
	}
	switch(op) {
	case '+':
		i += j;
		break;
	case '-':
		if (j > i) {
			fprintf(stderr, "do_binop: negative subtraction result\n");
			return -1;
		}
		i -= j;
		break;
	case '*':
		i *= j;
		break;
	case '/':
		if (j == 0) {
			fprintf(stderr, "do_binop: division by zero\n");
			return -1;
		}
		i /= j;
		break;
	}
	return i;
}

static int do_register_op(struct proc *p, int addr, char op, char *opname)
{
	char *temp;
	int res;
	
	temp = get_wordref(p, addr);
	if (temp == NULL) {
		fprintf(stderr, "%s: failed to get memory ref\n", opname);
		return -1;
	}
	res = do_binop(p->r, temp, op);
	if (res < 0) {
		fprintf(stderr, "%s: do_binop failed\n", opname);
		return -1;
	}
	release_wordref(p, addr);
	int2word(res, p->r);
	return 0;
}

int add(struct proc *p, int addr)
{
	return do_register_op(p, addr, '+', "add");
}

int subtract(struct proc *p, int addr)
{
	return do_register_op(p, addr, '-', "subtract");
}

int multiply(struct proc *p, int addr)
{
	return do_register_op(p, addr, '*', "multiply");
}

int divide(struct proc *p, int addr)
{
	return do_register_op(p, addr, '/', "divide");
}
