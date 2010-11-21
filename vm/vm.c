#include <string.h>
#include <stdio.h>
#include "vm.h"
#include "sem.h"
#include "mem.h"
#include "sched.h"
#include "wait_queue.h"
#include "loader.h"

struct msg_queues
{
	struct plist_head senders;
	struct plist_head recvers;
};

/* wait queues for message passing */
/* Note: last position is used to store procs receiving from any */
static struct msg_queues msg_wq[11];
static struct plist_head sem_wq[100];

void vm_init()
{
	memset(msg_wq, 0, 10*sizeof(struct msg_queues));
	memset(sem_wq, 0, 100*sizeof(struct plist_head));
}

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

static int load_register(struct proc *p, int addr)
{
	 if (addr < 0 || addr > 99) {
		fprintf(stderr, "load_register: invalid address\n");
		return -1;
	}
	if (load(p, addr, p->r) == -1) {
		fprintf(stderr, "load_register: load failed\n");
		return -1;
	}
	return 0;
}

static int load_low(struct proc *p, int addr)
{
	char temp[4];

	if (addr < 0 || addr > 99) {
		fprintf(stderr, "load_low: invalid address\n");
		return -1;
	}
	if (load(p, addr, temp) == -1) {
		fprintf(stderr, "load_low: load failed\n");
		return -1;
	}
	memcpy(p->r+2, temp+2, 2);
	return 0;
}

static int load_high(struct proc *p, int addr)
{
	char temp[4];

	if (addr < 0 || addr > 99) {
		fprintf(stderr, "load_high: invalid address\n");
		return -1;
	}
	if (load(p, addr, temp) == -1) {
		fprintf(stderr, "load_high: load failed\n");
		return -1;
	}
	memcpy(p->r, temp, 2);
	return 0;
}

static int store_register(struct proc *p, int addr)
{
	if (addr < 0 || addr > 99) {
		fprintf(stderr, "store_register: invalid address\n");
		return -1;
	}
	if (store(p, p->r, addr) == -1) {
		fprintf(stderr, "store_register: store failed\n");
		return -1;
	}
	return 0;
}

static int set_sp(struct proc *p, int addr)
{
	addr = word2int(p->r);
	if (addr < 0 || addr > 99) {
		fprintf(stderr, "set_sp: invalid address in register\n");
		return -1;
	}
	p->stack_base = addr;
	memcpy(p->sp, p->r, 4);
	return 0;
}

static int get_sp(struct proc *p, int addr)
{
	memcpy(p->r, p->sp, 4);
	return 0;
}

static int push(struct proc *p, int addr)
{
	addr = word2int(p->sp);
	if (addr == -1) {
		fprintf(stderr, "push: Non-numeric stack pointer\n");
		return -1;
	}
	addr++;
	int2word(addr, p->sp);
	if (addr > 99) {
		fprintf(stderr, "push: Out of memory\n");
		return -1;
	}
	if (store(p, p->r, addr) == -1) {
		fprintf(stderr, "push: store failed\n");
		return -1;
	}
	return 0;
}

static int pop(struct proc *p, int addr)
{
	addr = word2int(p->sp);
	if (addr == -1) {
		fprintf(stderr, "pop: non-numeric stack pointer\n");
		return -1;
	}
	if (addr > 99) {
		fprintf(stderr, "pop: Invalid stack pointer\n");
		return -1;
	}
	if (addr <= p->stack_base) {
		fprintf(stderr, "pop: pop on empty stack\n");
		return 0;
	}
	if (load(p, addr, p->r) == -1) {
		fprintf(stderr, "pop: load failed\n");
		return -1;
	}
	addr--;
	int2word(addr, p->sp);
	return 0;
}
	
static int cmp_eql(struct proc *p, int addr)
{
	char temp[4];

	if (addr < 0 || addr > 99) {
		fprintf(stderr, "cmp_eql: invalid address\n");
		return -1;
	}
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

static int cmp_less(struct proc *p, int addr)
{
	char temp[4];

	if (addr < 0 || addr > 99) {
		fprintf(stderr, "cmp_less: invalid address\n");
		return -1;
	}
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

static int jmp_if(struct proc *p, int addr)
{
	char temp[4];

	if (addr < 0 || addr > 99)
		return -1;

	int2word(addr, temp);
	if (p->c == 'T') {
		memcpy(p->ic, temp+2, 2);
	}
	return 0;
}

static int jmp(struct proc *p, int addr)
{
	char temp[4];

	if (addr < 0 || addr > 99) {
		fprintf(stderr, "jmp: invalid address\n");
		return -1;
	}
	int2word(addr, temp);
	memcpy(p->ic, temp+2, 2);
	return 0;
}

extern void read_word(char *buf);

static int read(struct proc *p, int addr)
{
	char temp[5];
	int i;

	if (addr < 0 || addr > 89) {
		fprintf(stderr, "read: invalid addresss\n");
		return -1;
	}
	for (i=0; i<10; i++) {
		read_word(temp);
		/* FIXME: this fails to read full words that begin with 'END' */
		if (strncasecmp(temp, "END", 3) == 0) {
			fprintf(stderr, "read: Out of input data\n");
			return -1;
		}
		if (feof(stdin)) {
			fprintf(stderr, "read: Unexpected EOF\n");
			return -1;
		}
		if (store(p, temp, addr++) == -1) {
			fprintf(stderr, "read: store failed\n");
			return -1;
		}
	}
	return 0;
}

static int print(struct proc *p, int addr)
{
	char temp[4];
	int i;

	if (addr < 0 || addr > 89) {
		fprintf(stderr, "print: invalid address\n");
		return -1;
	}

	for (i=0; i<10; i++) {
		if (load(p, addr++, temp) == -1) {
			fprintf(stderr, "print: load failed\n");
			return -1;
		}
		printf("%c%c%c%c\n", temp[0], temp[1], temp[2], temp[3]);
	}
	return 0;
}

static int add(struct proc *p, int addr)
{
	int i,j;
	char temp[4];

	if (addr < 0 || addr > 99) {
		fprintf(stderr, "add: invalid address\n");
		return -1;
	}
	if (load(p, addr, temp) == -1) {
		fprintf(stderr, "add: load failed\n");
		return -1;
	}
	i = word2int(temp);
	j = word2int(p->r);
	if (i == -1) {
		fprintf(stderr, "add: non-numeric word in memory\n");
		return -1;
	}
	if (j == -1) {
		fprintf(stderr, "add: non-numeric word in register\n");
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

	if (addr < 0 || addr > 99) {
		fprintf(stderr, "subtract: invalid address\n");
		return -1;
	}
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

	if (addr < 0 || addr > 99) {
		fprintf(stderr, "multiply: invalid address\n");
		return -1;
	}
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

	if (addr < 0 || addr > 99) {
		fprintf(stderr, "divide: invalid address\n");
		return -1;
	}
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

static int add_stack(struct proc *p, int addr)
{
	char word1[4];
	char word2[4];
	int i,j;

	addr = word2int(p->sp);
	if (addr < 0 || addr > 99) {
		fprintf(stderr, "add_stack: invalid stack pointer\n");
		return -1;
	}
	if (addr < p->stack_base+1) {
		fprintf(stderr, "add_stack: stack contains less than 2 elements\n");
		return -1;
	}

	if (load(p, addr--, word1) == -1) {
		fprintf(stderr, "add_stack: load failed\n");
		return -1;
	}
	if (load(p, addr, word2) == -1) {
		fprintf(stderr, "add_stack: load failed\n");
		return -1;
	}
	i = word2int(word1);
	j = word2int(word2);
	if (i == -1 || j == -1) {
		fprintf(stderr, "add_stack: non-numeric word on stack\n");
		return -1;
	}

	j += i;
	int2word(j, word2); /* store new stack value */
	int2word(addr, p->sp); /* store stack pointer */
	if (store(p, word2, addr) == -1) {
		fprintf(stderr, "add_stack: store failed\n");
		return -1;
	}
	return 0;
}

static int subtract_stack(struct proc *p, int addr)
{
	char word1[4];
	char word2[4];
	int i,j;

	addr = word2int(p->sp);
	if (addr < 0 || addr > 99) {
		fprintf(stderr, "subtract_stack: invalid stack pointer\n");
		return -1;
	}
	if (load(p, addr--, word1) == -1) {
		fprintf(stderr, "subtract_stack: load failed\n");
		return -1;
	}
	if (load(p, addr, word2) == -1) {
		fprintf(stderr, "subtract_stack: load_failed\n");
		return -1;
	}

	i = word2int(word1);
	j = word2int(word2);
	if (i==-1 || j==-1) {
		fprintf(stderr, "subtract_stack: non-numeric word on stack\n");
		return -1;
	}
	if (i > j) {
		fprintf(stderr, "subtract_stack: negative result\n");
		return -1;
	}

	j -= i;
	int2word(j, word2);
	int2word(addr, p->sp);
	if (store(p, word2, addr) == -1) {
		fprintf(stderr, "subtract_stack: store failed\n");
		return -1;
	}
	return 0;
}

static int multiply_stack(struct proc *p, int addr)
{
	char word1[4];
	char word2[4];
	int i,j;

	addr = word2int(p->sp);
	if (addr < 0 || addr > 99) {
		fprintf(stderr, "multiply_stack: invalid stack pointer\n");
		return -1;
	}
	if (load(p, addr--, word1) == -1) {
		fprintf(stderr, "multiply_stack: load failed\n");
		return -1;
	}
	if (load(p, addr, word2) == -1) {
		fprintf(stderr, "multiply_stack: load failed\n");
		return -1;
	}

	i = word2int(word1);
	j = word2int(word2);
	if (i==-1 || j==-1) {
		fprintf(stderr, "multiply_stack: non-numeric word on stack\n");
		return -1;
	}

	j *= i;
	int2word(j, word2);
	int2word(addr, p->sp);
	if (store(p, word2, addr) == -1) {
		fprintf(stderr, "multiply_stack: store failed\n");
		return -1;
	}
	return 0;
}

static int divide_stack(struct proc *p, int addr)
{
	char word1[4];
	char word2[4];
	int i,j;

	addr = word2int(p->sp);
	if (addr < 0 || addr > 99) {
		fprintf(stderr, "divide_stack: invalid stack pointer\n");
		return -1;
	}
	if (load(p, addr--, word1) == -1) {
		fprintf(stderr, "divide_stack: load failed\n");
		return -1;
	}
	if (load(p, addr, word2) == -1) {
		fprintf(stderr, "divide_stack: load failed\n");
		return -1;
	}

	i = word2int(word1);
	j = word2int(word2);

	if (i==-1 || j==-1) {
		fprintf(stderr, "divide_stack: non-numeric word on stack\n");
		return -1;
	}
	if (i == 0) {
		fprintf(stderr, "divide_stack: division by zero\n");
		return -1;
	}

	j /= i;
	int2word(j, word2);
	int2word(addr, p->sp);
	if (store(p, word2, addr) == -1) {
		fprintf(stderr, "divide_stack: store failed\n");
		return -1;
	}
	return 0;
}

static int nop(struct proc *p, int addr)
{
	return 0;
}

static struct plist *get_recver(struct proc *sender, int target_pid)
{
	struct plist *waitp;

	/* check receievers waiting on us */
	for (waitp = msg_wq[sender->pid].recvers.head; waitp != NULL; waitp = waitp->next) {
		if (waitp->p->pid == target_pid)
			return waitp;
	}
	/* check receivers waiting on anybody */
	for (waitp = msg_wq[10].recvers.head; waitp != NULL; waitp = waitp->next) {
		if (waitp->p->pid == target_pid)
			return waitp;
	}
	return NULL;
}

static int send(struct proc *p, int pid)
{
	struct plist *waitp;

	if (pid < 0 || pid > 99) {
		fprintf(stderr, "send: invalid process id\n");
		return 1;
	}

	/* Block on the receiver */
	sched_suspend(p->pid);
	if (insert_proc(&(msg_wq[pid].senders), p) != 0) {
		fprintf(stderr, "send: failed to add self to wait queue\n");
		return 1;
	}

	/* wake up the receiever if there is one */
	waitp = get_recver(p, pid);
	if (waitp == NULL)
		return 0;
	/* remove receiver from either wait queue (ours or the any queue) */
	if (remove_proc(&(msg_wq[p->pid].recvers), waitp->p) != 0 &&
	    remove_proc(&(msg_wq[10].recvers), waitp->p) != 0) {
		fprintf(stderr, "send: receiver appears to not be in either wait queue...\n");
		return 1;
	}
	sched_resume(pid);
	return 0;
}

static int rewind_ic(struct proc *p)
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

static int get_msg(struct proc *sender, struct proc *recver)
{
	int store_loc, load_loc;
	int i;
	char temp[4];

	load_loc = word2int(sender->r);
	store_loc = word2int(recver->r);
	if (load_loc < 0) {
		fprintf(stderr, "get_msg: invalid load location ");
		print_word(stderr, sender->r);
		fprintf(stderr, "\n");
		return 1;
	}
	if (store_loc < 0) {
		fprintf(stderr, "get_msg: invalid store location ");
		print_word(stderr, recver->r);
		fprintf(stderr, "\n");
		return 1;
	}
	for (i=0; i<10; i++) {
		if (load(sender, load_loc++, temp) != 0) {
			fprintf(stderr, "get_msg: load failed\n");
			return 1;
		}
		if (store(recver, temp, store_loc++) != 0) {
			fprintf(stderr, "get_msg: store failed\n");
			return 1;
		}
	}
	return 0;
}

static int recv(struct proc *p, int pid)
{
	struct plist *waitp;
	unsigned int index;

	if (pid > 99) {
		fprintf(stderr, "recv: invalid pid (pid %u)\n", p->pid);
		return 1;
	}

	/* get the waiting process, if any */
	waitp = msg_wq[p->pid].senders.head;
	if (pid >= 0) {
		for (; waitp!=NULL; waitp=waitp->next) {
			if (waitp->p->pid == pid)
				break;
		}
	}

	if (waitp == NULL) {
		if (rewind_ic(p) != 0) {
			fprintf(stderr, "recv: invalid IC\n");
			return 1;
		}
		/* put ourselves on the wait queue and suspend */
		index = pid;
		if (pid < 0)
			index = 10;
		sched_suspend(p->pid);
		if (insert_proc(&(msg_wq[index].recvers), p) != 0) {
			fprintf(stderr, "recv: Failed to add self to wait queue\n");
			return -1;
		}
		return 0;
	}

	if (get_msg(waitp->p, p) != 0) {
		fprintf(stderr, "recv: get_msg failed (pid %u)\n", p->pid);
		return 1;
	}
	/* we're done with the sender, so wake them up */
	pid = waitp->p->pid;
	if (remove_proc(&(msg_wq[p->pid].senders), waitp->p) != 0) {
		fprintf(stderr, "recv: failed to remove sender from wait queue\n");
		return 1;
	}
	sched_resume(pid);
	return 0;
}

static int set_sem(struct proc *p, int sem)
{
	int val;

	val = word2int(p->r);
	if (val < 0) {
		fprintf(stderr, "set_sem: Invalid semaphor value ");
		print_word(stderr, p->r);
		fprintf(stderr, " (pid %u)\n", p->pid);
		return 1;
	}
	if (sem < 0) {
		fprintf(stderr, "set_sem: Invalid semaphor (pid %u)\n", p->pid);
		return 1;
	}
	if (init_sem(sem, val) != 0) {
		fprintf(stderr, "set_sem: Error setting semaphor (pid %u)\n", p->pid);
		return 1;
	}
	return 0;
}

static int sem_down(struct proc *p, int sem)
{
	if (sem < 0) {
		fprintf(stderr, "sem_down: Invalid semaphor (pid %u)\n", p->pid);
		return 1;
	}
	switch (sem_p(sem)) {
	case -1:
		fprintf(stderr, "sem_down: error putting semaphor down (pid %u)\n", p->pid);
		return 1;
	case 0:
		return 0;
	case 1:
		if (rewind_ic(p) != 0) {
			fprintf(stderr, "sem_down: error resetting IC (pid %u)\n", p->pid);
			return 1;
		}
		sched_suspend(p->pid);
		if (insert_proc(&sem_wq[sem], p) != 0) {
			fprintf(stderr, "sem_down: error adding self to wait queue (pid %u)\n", p->pid);
			return 1;
		}
		return 0;
	default:
		fprintf(stderr, "sem_down: unexpected value returned from p() (pid %u)\n", p->pid);
		return 1;
	}
}

static int sem_up(struct proc *p, int sem)
{
	struct plist *waitp;

	if (sem < 0) {
		fprintf(stderr, "sem_up: invalid semaphor (pid %u)\n", p->pid);
		return 1;
	}
	if (sem_v(sem) != 0) {
		fprintf(stderr, "sem_up: error putting semaphor up (pid %u)\n", p->pid);
		return 1;

	}
	/* wake up the first waiting process, if any */
	waitp = sem_wq[sem].head;
	if (waitp == NULL)
		return 0;
	sched_resume(waitp->p->pid);
	if (remove_proc(&sem_wq[sem], waitp->p) != 0) {
		fprintf(stderr, "sem_up: error removing waiting proc from wait queue (pid %u)\n", p->pid);
		return 1;
	}
	return 0;
}

static int load_shared(struct proc *p, int addr)
{
	if (addr < 0) {
		fprintf(stderr, "load_shared: invalid address (pid %u)\n", p->pid);
		return 1;
	}
	if (load_shm(addr, p->r) != 0) {
		fprintf(stderr, "load_shared: load failed (pid %u)\n", p->pid);
		return 1;
	}
	return 0;
}

static int store_shared(struct proc *p, int addr)
{
	if (addr < 0) {
		fprintf(stderr, "store_shared: invalid address (pid %u)\n", p->pid);
		return 1;
	}
	if (store_shm(p->r, addr) != 0) {
		fprintf(stderr, "store_shared: store failed (pid %u)\n", p->pid);
		return 1;
	}
	return 0;
}

static int fork_proc(struct proc *p, int addr)
{
	struct proc *newp;
	int procsize;
	char temp[4];
	int i;

	procsize = word2int(p->lr);
	if (procsize < 0) {
		fprintf(stderr, "fork_proc: bad limit register (pid %u)\n", p->pid);
		return 1;
	}
	newp = procalloc(procsize);
	if (newp == NULL) {
		memset(p->r, '0', 4);
	} else {
		int2word(newp->pid, p->r);
		int2word(p->pid, newp->r);
	}
	memcpy(newp->ic, p->ic, 2);
	newp->c = p->c;
	newp->stack_base = p->stack_base;

	/* duplicate memory */
	for (i=0; i<procsize; i++) {
		if (load(p, i, temp) != 0) {
			fprintf(stderr, "fork_proc: load failed (pid %u)\n", p->pid);
			return 1;
		}
		if (store(newp, temp, i) != 0) {
			fprintf(stderr, "fork_proc: store failed (pid %u)\n", p->pid);
			return 1;
		}
	}
	sched_reset(newp->pid);
	sched_resume(newp->pid);
	return 0;
}

static int exec_proc(struct proc *p, int addr)
{
	int ic, procsize;
	char fname[9] = "XX.brain";
	char temp[7];
	FILE *fh;

	memset(temp, '0', 4);
	memcpy(temp+2, p->ic, 2);
	ic = word2int(temp);
	if (ic < 0) {
		fprintf(stderr, "exec_proc: bad IC (pid %u)\n", p->pid);
		return 1;
	}
	/* get the filename */
	ic--;
	if (load(p, ic, temp) != 0) {
		fprintf(stderr, "exec_proc: load failed (pid %u)\n", p->pid);
		return 1;
	}
	memcpy(fname, temp+2, 2);

	fh = fopen(fname, "r");
	if (fh == NULL) {
		fprintf(stderr, "exec_proc: failed to open file '%s' (pid %u)\n", fname, p->pid);
		return 1;
	}
	if (fscanf(fh, "%7s", temp) != 1) {
		fprintf(stderr, "exec_proc: failed to read file header (pid %u)\n", p->pid);
		fclose(fh);
		return 1;
	}
	if (strncmp("BRAIN10", temp, 7) != 0) {
		fprintf(stderr, "exec_proc: bad file header (pid %u)\n", p->pid);
		return 1;
	}
	/* free this processes' memory */
	addr = word2int(p->br);
	procsize = word2int(p->lr);
	if (addr < 0 || procsize < 0) {
		fprintf(stderr, "exec_proc: bad base or limit register (pid %u)\n", p->pid);
		return 1;
	}
	freemem(addr, procsize);

	/* reallocate space for the process */
	procsize = load_header(fh);
	if (procsize < 0) {
		fprintf(stderr, "exec_proc: missing or bad process size header (pid %u)\n", p->pid);
		return 1;
	}

	addr = memalloc(procsize);
	if (addr < 0) {
		int2word(0, p->r);
		fclose(fh);
		return 0;
	}
	int2word(addr, p->br);
	int2word(procsize, p->lr);
	if (load_code(fh, p) < 0) {
		fprintf(stderr, "exec_proc: failed to load process code (pid %u)\n", p->pid);
		fclose(fh);
		return 1;
	}
	/* finally reset registers */
	memset(p->r, '0', 4);
	memset(p->ic, '0', 2);
	p->c = 'F';
	p->stack_base = 0;
	fclose(fh);
	return 0;
}	

static int identify(struct proc *p, int addr)
{
	int2word(p->pid, p->r);
	return 0;
}

static int halt(struct proc *p, int addr)
{
	int base, limit;

	base = word2int(p->br);
	limit=word2int(p->lr);
	if (base < 0 || limit < 0) {
		fprintf(stderr, "halt: bad base or limit register, not freeing memory\n");
	} else {
		freemem(base, limit);
	}
	sched_reset(p->pid);
	sched_suspend(p->pid);
	return 0;
}

static struct op op_table[] = {
	{ .opcode=LR, .run=load_register },
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

struct proc proc_table[100];
static unsigned int next_pid=0;

struct proc *procalloc(unsigned int procsize)
{
	struct proc *p;
	int base_addr;

	if (next_pid > 99) {
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
	p->pid = next_pid;
	int2word(base_addr, p->br);
	int2word(procsize, p->lr);
	next_pid++;
	return p;
}

int tick(unsigned int pid)
{
	char word[4];
	char temp[4];
	enum OP op;
	int addr, ic, i;
	struct proc *p;

	if (pid >= next_pid) {
		fprintf(stderr, "tick: invalid PID %u\n", pid);
		return 1;
	}
	p = &proc_table[pid];

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
	if (load(p, ic, word) == -1) {
		fprintf(stderr, "tick: load failed\n");
		return -1;
	}

	/* increment p->ic */
	ic++;
	int2word(ic, temp);
	memcpy(p->ic, temp+2, 2);

	/* execute the instruction */
	op = OPCODE(word[0], word[1]);
	memset(temp, '0', 4);
	memcpy(temp+2, word+2, 2);
	addr = word2int(temp);

	for (i=0; i<LEN(op_table); i++) {
		if (op_table[i].opcode == op)
			return op_table[i].run(p, addr);
	}
	fprintf(stderr, "tick: Illegal instruction word %c%c%c%c (pid %u)\n", word[0], word[1], word[2], word[3], pid);
/*	fprintf(stderr, "tick: Illegal operation %c%c (pid %u, IC %c%c)\n", word[0], word[1], pid, p->ic[0], p->ic[1]); */
	return -1;
}
