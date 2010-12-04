#include <string.h>
#include <stdio.h>
#include "vm.h"
#include "sem.h"
#include "mem.h"
#include "sched.h"
#include "wait_queue.h"
#include "loader.h"
#include "ops.h"


int word2int(char *p)
{
	int i,j;

	i=0;
	for (j=0; j<4; j++) {
		if (p[j] < '0' || p[j] > '9')
			return -1;
		i = i*10+p[j]-'0';
	}
	return i;
}

void int2word(int a, char *p)
{
	int i;

	for (i=3; i>=0; i--) {
		p[i] = (a % 10) + '0';
		a /= 10;
	}
}

/* have this process retry the current operation next */
int retry_op(struct proc *p)
{
	int ic;
	char temp[4];

	memset(temp, '0', 4);
	memcpy(temp+2, p->ic, 2);
	ic = word2int(temp);
	if (ic < 0)
		return 1;
	int2word(--ic, temp);
	memcpy(p->ic, temp+2, 2);
	return 0;
}

static struct op op_table[] = {
	{ .opcode=LR, .run=load_all },
	{ .opcode=LL, .run=load_low },
	{ .opcode=LH, .run=load_high },
	{ .opcode=SR, .run=store_register },
	{ .opcode=SP, .run=set_sp },
	{ .opcode=PS, .run=get_sp },
	{ .opcode=PH, .run=push },
	{ .opcode=PP, .run=pop },
	{ .opcode=CE, .run=cmp_eql },
	{ .opcode=CL, .run=cmp_less },
	{ .opcode=BT, .run=jmp_if },
	{ .opcode=BU, .run=jmp },
	{ .opcode=GD, .run=read },
	{ .opcode=PD, .run=print },
	{ .opcode=AD, .run=add },
	{ .opcode=SU, .run=subtract },
	{ .opcode=MI, .run=multiply },
	{ .opcode=DI, .run=divide },
	{ .opcode=AS, .run=add_stack },
	{ .opcode=SS, .run=subtract_stack },
	{ .opcode=MS, .run=multiply_stack },
	{ .opcode=DS, .run=divide_stack },
	{ .opcode=NP, .run=nop },
	{ .opcode=SD, .run=send },
	{ .opcode=RC, .run=recv },
	{ .opcode=SI, .run=set_sem },
	{ .opcode=PE, .run=sem_down },
	{ .opcode=VE, .run=sem_up },
	{ .opcode=LS, .run=load_shared },
	{ .opcode=ST, .run=store_shared },
	{ .opcode=FK, .run=fork_proc },
	{ .opcode=EX, .run=exec_proc },
	{ .opcode=GP, .run=identify },
	{ .opcode=HA, .run=halt }
};

struct proc proc_table[PROCS];
static unsigned int next_pid=0;

struct proc *procalloc(unsigned int procsize)
{
	struct proc *p;
	int base_addr;

	if (next_pid > PROCS-1) {
		fprintf(stderr, "procalloc: out of PIDs (!!)\n");
		return NULL;
	}
	base_addr=memalloc(procsize);
	if (base_addr < 0) {
		fprintf(stderr, "procalloc: out of memory\n");
		return NULL;
	}
	p=&proc_table[next_pid];
	memset(p, '0', sizeof(struct proc));
	p->c = 'F';
	p->stack_base = 0;
	p->pid = next_pid++;
	int2word(base_addr, p->br);
	int2word(procsize-1, p->lr);
	return p;
}

int tick(unsigned int pid)
{
	char *word;
	char temp[4];
	enum OP op;
	int addr, ic, i;
	int res;
	struct proc *p;

	if (pid >= next_pid) {
		fprintf(stderr, "tick: invalid PID %u\n", pid);
		return 1;
	}
	p = &proc_table[pid];

	if (p->pid != pid) {
		fprintf(stderr, "tick: pid table entry does not match pid\n");
		return -1;
	}

	/* load the word at p->ic */
	memset(temp, '0', 4);
	memcpy(temp+2, p->ic, 2);
	ic = word2int(temp);
	if (ic == -1) {
		fprintf(stderr, "tick: invalid IC ");
		print_word(stderr, temp);
		fprintf(stderr, " (pid %u)\n", p->pid);
		return -1;
	}
	word = get_wordref(p, ic);
	if (word == NULL) {
		fprintf(stderr, "tick: failed to get memory ref\n");
		return -1;
	}

	/* increment p->ic */
	int2word(ic+1, temp);
	memcpy(p->ic, temp+2, 2);

	/* execute the instruction */
	op = OPCODE(word[0], word[1]);
	memset(temp, '0', 4);
	memcpy(temp+2, word+2, 2);
	addr = word2int(temp);

	for (i=0; i<LEN(op_table); i++) {
		if (op_table[i].opcode == op) {
			res = op_table[i].run(p, addr);
			/* note: ic has been incremented by now */
			release_wordref(p, ic);
			return res;
		}
	}
	print_mem(p);
	fprintf(stderr, "tick: Illegal instruction word %c%c%c%c (pid %u)\n", word[0], word[1], word[2], word[3], pid);
/*	fprintf(stderr, "tick: Illegal operation %c%c (pid %u, IC %c%c)\n", word[0], word[1], pid, p->ic[0], p->ic[1]); */
	return -1;
}
