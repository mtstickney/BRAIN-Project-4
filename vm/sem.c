#include <stdio.h>
#include "sem.h"
#include "vm.h"
#include "wait_queue.h"
#include "ops.h"
#include "sched.h"
#include "mem.h"

#define NSEMS 100

static struct plist_head sem_wq[NSEMS];

void sem_op_init()
{
	int i;

	for (i=0; i<NSEMS; i++)
		sem_wq[i].head = sem_wq[i].tail = NULL;
	sem_init();
}

int set_sem(struct proc *p, int sem)
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
	if (sem_set(sem, val) != 0) {
		fprintf(stderr, "set_sem: Error setting semaphor (pid %u)\n", p->pid);
		return 1;
	}
	return 0;
}

int sem_down(struct proc *p, int sem)
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
		if (retry_op(p) != 0) {
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

int sem_up(struct proc *p, int sem)
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
