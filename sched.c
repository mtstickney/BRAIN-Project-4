#include "red_black_tree.h"
#include "sched.h"

#define SCHED_GRANULARITY 20
#define SCHED_LATENCY 2

#define MAX(a,b) ((a) > (b)) ? (a) : (b)

struct proc;
extern int tick(unsigned int pid);

struct sched_ent
{
	unsigned int pid;
	unsigned int vruntime;
	rb_red_blk_node *n;
};

struct cfs_rq
{
	rb_red_blk_tree *t;
	unsigned int min_vruntime;
	unsigned int current;
	unsigned int nr_running;
};

static struct sched_ent task_table[100];

static struct cfs_rq rq = {.t=NULL, .min_vruntime=0, .current=0, .nr_running=0};

/* accounting vars for program analysis */
static unsigned int ctx_switches;
unsigned int ops;

static rb_red_blk_node *leftmost(rb_red_blk_tree *t)
{
	rb_red_blk_node *n;

	for (n = t->root; n->left != t->nil; n = n->left);
	return n;
}

static int key_comp(const void* k1, const void* k2)
{
	int x1, x2;

	/* avoid vruntime overflow by subtracting min_vruntime */
	x1 = *((unsigned int*)k1)-rq.min_vruntime;
	x2 = *((unsigned int*)k2)-rq.min_vruntime;
	if (x1 == x2)
		return 0;
	return x1 < x2 ? -1 : 1;
}

int sched_init()
{
	int i;

	ops = 0;
	ctx_switches=0;
	rq.t = RBTreeCreate(key_comp, NullFunction, NullFunction, NullFunction, NullFunction);
	if (rq.t == NULL)
		return 1;
	for (i=0; i<100; i++) {
		task_table[i].pid = i;
		task_table[i].vruntime=0;
		task_table[i].n=NULL;
	}
	return 0;
}

/* context switch to the next process.
 * note that the scheduler is not responsible for
 * saving the current process somewhere. */
static void ctxt_switch()
{
	struct sched_ent *newp;

	rb_red_blk_node *n;
	n = leftmost(rq.t);
	newp = (struct sched_ent*)n->info;
	rq.current = newp->pid;
	task_table[rq.current].n = NULL;
	RBDelete(rq.t, n);

	ctx_switches++;
	fprintf(stderr, "ACCT: %u ctxt switches\n", ctx_switches);
}

int sched_suspend(unsigned int pid)
{
	struct sched_ent *taskp;

	if (pid > 99) {
		fprintf(stderr, "sched_suspend: invalid PID %u\n", pid);
		return 1;
	}

	taskp = &task_table[pid];
	if (rq.current == pid) {
		if (rq.nr_running > 1)
			ctxt_switch();
	} else {
		RBDelete(rq.t, taskp->n);
		taskp->n = NULL;
	}
	rq.nr_running--;
	return 0;
}

int sched_resume(unsigned int pid)
{
	struct sched_ent *newp, *curp;
	rb_red_blk_node *n;

	if (pid > 99) {
		fprintf(stderr, "sched_resume: invalid PID %u\n", pid);
		return 1;
	}

	newp = &task_table[pid];
	curp = &task_table[rq.current];
	/* don't let a process that has slept for an hour starve all the others */
	if (rq.min_vruntime < SCHED_LATENCY) {
		newp->vruntime = MAX(newp->vruntime, rq.min_vruntime);
	} else {
		newp->vruntime = MAX(newp->vruntime, rq.min_vruntime-SCHED_LATENCY);
	}

	if (rq.nr_running == 0) {
		rq.min_vruntime = newp->vruntime;
		rq.current = newp->pid;
		rq.nr_running++;
		return 0;
	}
	if (pid == rq.current) {
		fprintf(stderr, "sched_resume: trying to resume the active process. Suspicious...\n");
		return 0;
	}

	newp->n = RBTreeInsert(rq.t, &(newp->vruntime), newp);
	n = leftmost(rq.t);
	if (key_comp(&(curp->vruntime), n->key) > 0) {
		curp->n = RBTreeInsert(rq.t, &(curp->vruntime), curp);
		ctxt_switch();
	}
	rq.nr_running++;
	return 0;
}

/* Note: a sched_suspend *must* be called after a reset */
int sched_reset(unsigned int pid)
{
	if (pid > 99) {
		fprintf(stderr, "reset: invalid PID %u\n", pid);
		return 1;
	}
	task_table[pid].vruntime = 0;
	return 0;
}

int sched_run()
{
	unsigned int i;
	rb_red_blk_node *n;
	struct sched_ent *curp;

	while (rq.nr_running > 0) {
		for (i=0; i<SCHED_GRANULARITY && rq.nr_running > 0; i++) {
			/* note that a tick may context switch the current process */
			task_table[rq.current].vruntime++;
			if (tick(rq.current) != 0)
				return -1;
			fprintf(stderr, "ACCT: %u ops\n", ops++);
		}
		if (rq.nr_running < 2)
			continue;
		n = leftmost(rq.t);
		curp = &task_table[rq.current];
		if (key_comp(&(curp->vruntime), n->key) > 0) {
			/* no longer the most unfairly treated proc, time for context switch */
			rq.min_vruntime = *((unsigned int*)n->key);
			curp->n = RBTreeInsert(rq.t, &(curp->vruntime), curp);
			ctxt_switch();
		}
	}
	return 0;
}
