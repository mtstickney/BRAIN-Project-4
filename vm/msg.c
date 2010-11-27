#include <stdio.h>
#include "mem.h"
#include "vm.h"
#include "wait_queue.h"
#include "sched.h"
#include "ops.h"

/* send and recv waitqueues (index PID owns queue) */
/* last recv_wq index is the 'any' waitqueue */
static struct plist_head recv_wq[PROCS+1];
static struct plist_head send_wq[PROCS];

static int xfer_msg(struct proc *sender, struct proc *recver)
{
	int store_loc, load_loc;
	int i;
	char temp[4];

	load_loc = word2int(sender->r);
	store_loc = word2int(recver->r);
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

void msg_init()
{
	int i;

	for (i=0; i<PROCS; i++) {
		recv_wq[i].head = recv_wq[i].tail = NULL;
		send_wq[i].head = send_wq[i].tail = NULL;
	}
	recv_wq[PROCS].head = recv_wq[PROCS].tail = NULL;
}

int recv(struct proc *p, int pid)
{
	struct plist *senderp;
	unsigned int index;

	if (pid > PROCS-1) {
		fprintf(stderr, "recv: invalid pid (pid %u)\n", p->pid);
		return 1;
	}

	/* get the waiting process, if any */
	senderp = send_wq[p->pid].head;
	for (; senderp != NULL; senderp=senderp->next) {
		if (pid < 0 || senderp->p->pid == pid)
			break;
	}

	if (senderp == NULL) {
		if (retry_op(p) != 0) {
			fprintf(stderr, "recv: invalid IC\n");
			return 1;
		}
		/* put ourselves on the wait queue and suspend */
		index = pid < 0 ? PROCS : pid;
		if (insert_proc(&recv_wq[index], p) != 0) {
			fprintf(stderr, "recv: Failed to add self to wait queue\n");
			return -1;
		}
		sched_suspend(p->pid);
		return 0;
	}

	/* get the message */
	if (xfer_msg(senderp->p, p) != 0) {
		fprintf(stderr, "recv: xfer_msg failed (pid %u)\n", p->pid);
		return 1;
	}
	/* wake up the sender */
	pid = senderp->p->pid;
	if (remove_proc(&send_wq[p->pid], senderp->p) != 0) {
		fprintf(stderr, "recv: failed to remove sender from wait queue\n");
		return 1;
	}
	sched_resume(pid);
	return 0;
}

int send(struct proc *p, int pid)
{
	struct plist *recvp;

	if (pid < 0 || pid > PROCS-1) {
		fprintf(stderr, "send: invalid process id\n");
		return 1;
	}

	/* Block on the receiver */
	sched_suspend(p->pid);
	if (insert_proc(&send_wq[pid], p) != 0) {
		fprintf(stderr, "send: failed to add self to wait queue\n");
		return 1;
	}

	/* wake up the receiever if there is one */
	recv_wq[p->pid].tail->next = recv_wq[PROCS].head; /* temporarily combine wait queues */
	recvp = recv_wq[p->pid].head;
	for (; recvp != NULL; recvp=recvp->next) {
		if (recvp->p->pid == pid)
			break;
	}
	recv_wq[p->pid].tail->next = NULL; /* split queues again */

	if (recv_wq == NULL)
		return 0;

	/* remove receiver from either wait queue (ours or the any queue) */
	if (remove_proc(&recv_wq[p->pid], recvp->p) != 0 &&
	    remove_proc(&recv_wq[PROCS], recvp->p) != 0) {
		fprintf(stderr, "send: receiver appears to not be in either wait queue...\n");
		return -1;
	}
	sched_resume(pid);
	return 0;
}