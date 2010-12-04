#include <stdio.h>
#include <string.h>
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
	char *send_word;
	char *recv_word;

	load_loc = word2int(sender->r);
	store_loc = word2int(recver->r);
	for (i=0; i<10; i++) {
		send_word = get_wordref(sender, load_loc+i);
		if (send_word == NULL) {
			fprintf(stderr, "get_msg: failed to get memory ref\n");
			return 1;
		}
		recv_word = get_wordref(recver, store_loc+i);
		if (recv_word == NULL) {
			fprintf(stderr, "get_msg: failed to get memory ref\n");
			return 1;
		}
		memcpy(recv_word, send_word, 4);
		release_wordref(sender, load_loc+i);
		release_wordref(recver, store_loc+i);
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
	struct plist_head *wq;

	if (pid < 0 || pid > PROCS-1) {
		fprintf(stderr, "send: invalid process id\n");
		return 1;
	}

	/* Block on the receiver */
	sched_suspend(p->pid);
	if (insert_proc(&send_wq[pid], p) != 0) {
		fprintf(stderr, "send: failed to add self to wait queue\n");
		return -1;
	}

	/* wake up the receiever if there is one */
	wq = &recv_wq[p->pid];
	recvp = wq->head;
	while (recvp != NULL || wq != &recv_wq[PROCS]) {
		if (recvp == NULL) {
			wq = &recv_wq[PROCS];
			recvp = wq->head;
			continue;
		}
		if (recvp->p->pid == pid)
			break;
		recvp=recvp->next;
	}
	if (recvp == NULL)
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
