#include <stdio.h>
#include <string.h>
#include "loader.h"
#include "vm.h"
#include "mem.h"
#include "ops.h"

static FILE *get_exec_fh(struct proc *p)
{
	int ic;
	char temp[4];
	char *word;
	char fname[9] = "XX.brain";

	memset(temp, '0', 4);
	memcpy(temp+2, p->ic, 2);
	ic = word2int(temp);
	if (ic < 0)
		return NULL;
	/* tick() already incremented for us... */
	ic--;
	word = get_wordref(p, ic);
	if (word == NULL)
		return NULL;
	memcpy(fname, word+2, 2);
	release_wordref(p, ic);
	return fopen(fname, "r");
}

int exec_proc(struct proc *p, int addr)
{
	int procsize;
	char temp[7];
	FILE *fh;

	fh = get_exec_fh(p);
	if (fh == NULL) {
		fprintf(stderr, "exec_proc: failed to open file for exec\n");
		return 1;
	}
	if (fscanf(fh, "%7s", temp) != 1 || strncmp("BRAIN10", temp, 7) != 0) {
		fprintf(stderr, "exec_proc: failed to read file header (pid %u)\n", p->pid);
		goto ERR_OUT;
	}

	/* try to alloc a new process */
	procsize = load_header(fh);
	if (procsize < 0) {
		fprintf(stderr, "exec_proc: missing or bad process size header\n");
		goto ERR_OUT;
	}
	if (resize_proc(p, procsize) != 0) {
		memset(p->r, '0', 4);
		return 0;
	}
	if (load_code(fh, p) == -1) {
		fprintf(stderr, "exec_proc: failed to load program code\n");
		goto ERR_OUT;
	}
	/* reset process state */
	memset(p->r, '0', 4);
	memset(p->ic, '0', 2);
	memset(p->sp, '0', 4);
	p->c = 'F';
	p->stack_base = 0;
	fclose(fh);
	return 0;

ERR_OUT:
	fclose(fh);
	return -1;
}
