#include <stdio.h>
#include "vm.h"
#include "mem.h"
#include "ops.h"

static int do_binop(struct proc *p, int addr, char op)
{
	int i,j;
	char temp[4];
	
	if (load(p, addr, temp) == -1) {
		fprintf(stderr, "do_binop: load failed\n");
		return -1;
	}
	i = word2int(p->r);
	j = word2int(temp);
	if (i == -1) {
		fprintf(stderr, "do_binop: non-numeric word in memory\n");
		return -1;
	}
	if (j == -1) {
		fprintf(stderr, "do_binop: non-numeric word in register\n");
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
	int2word(i, p->r);
	return 0;
}

int add(struct proc *p, int addr)
{
	if (do_binop(p, addr, '+') != 0) {
		fprintf(stderr, "add: do_binop failed\n");
		return -1;
	}
	return 0;
}

int subtract(struct proc *p, int addr)
{
	if (do_binop(p, addr, '-') != 0) {
		fprintf(stderr, "subtract: do_binop failed\n");
		return -1;
	}
	return 0;
}

int multiply(struct proc *p, int addr)
{
	if (do_binop(p, addr, '*') != 0) {
		fprintf(stderr, "multiply: do_binup failed\n");
		return -1;
	}
	return 0;
}

int divide(struct proc *p, int addr)
{
	if (do_binop(p, addr, '/') != 0) {
		fprintf(stderr, "divide: do_binup failed\n");
		return -1;
	}
	return 0;
}		

#if 0
static int add(struct proc *p, int addr)
{
	int i,j;
	char temp[4];

	if (load(p, addr, temp) == -1) {
		fprintf(stderr, "add: load failed\n");
		return -1;
	}
	i = word2int(temp);
	j = word2int(p->r);
	if (i == -1) {
		fprintf(stderr, "add: non-numeric word in memory: ");
		print_word(stderr, temp);
		fprintf(stderr, "\n");
		return -1;
	}
	if (j == -1) {
		fprintf(stderr, "add: non-numeric word in register: ");
		print_word(stderr, p->r);
		fprintf(stderr, "\n");
		return -1;
	}
	i += j;
	int2word(i, p->r);
	return 0;
}

static int subtract(struct proc *p, int addr)
{
	char temp[4];
	int i,j;

	if (load(p, addr, temp) == -1) {
		fprintf(stderr, "subtract: load failed\n");
		return -1;
	}
	
	i = word2int(p->r);
	j = word2int(temp);
	if (i == -1) {
		fprintf(stderr, "subtract: non-numeric word in register\n");
		return -1;
	}
	if (j == -1) {
		fprintf(stderr, "subtract: non-numeric word in memory\n");
		return -1;
	}
	if (i < j) {
		fprintf(stderr, "subtract: negative result\n");
		return -1;
	}
	i -= j;
	int2word(i, p->r);
	return 0;
}

static int multiply(struct proc *p, int addr)
{
	char temp[4];
	int i,j;

	if (load(p, addr, temp) == -1) {
		fprintf(stderr, "multiply: load failed\n");
		return -1;
	}

	i = word2int(p->r);
	j = word2int(temp);
	if (i == -1) {
		fprintf(stderr, "multiply: non-numeric word in register\n");
		return -1;
	}
	if (j == -1) {
		fprintf(stderr, "multiply: non-numeric word in memory\n");
		return -1;
	}

	i *= j;
	int2word(i, p->r);
	return 0;
}

static int divide(struct proc *p, int addr)
{
	char temp[4];
	int i,j;

	if (load(p, addr, temp) == -1) {
		fprintf(stderr, "divide: load failed\n");
		return -1;
	}

	i = word2int(p->r);
	j = word2int(temp);
	if (i == -1) {
		fprintf(stderr, "divide: non-numeric word in register\n");
		return -1;
	}
	if (j == -1) {
		fprintf(stderr, "divide: non-numeric word in memory\n");
		return -1;
	}
	if (j == 0) {
		fprintf(stderr, "divide: division by zero\n");
		return -1;
	}
	i /= j;
	int2word(i, p->r);
	return 0;
}
#endif