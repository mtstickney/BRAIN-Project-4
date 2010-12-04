#include <stdio.h>
#include <string.h>
#include "vm.h"
#include "mem.h"
#include "ops.h"

int load_all(struct proc *p, int addr)
{
	char *word;

	word = get_wordref(p, addr);
	if (word == NULL) {
		fprintf(stderr, "load_all: failed to get memory ref\n");
		return -1;
	}
	memcpy(p->r, word, 4);
	release_wordref(p, addr);
	return 0;
}

int load_low(struct proc *p, int addr)
{
	char *temp;

	temp = get_wordref(p, addr);
	if (temp == NULL) {
		fprintf(stderr, "load_all: failed to get memory ref\n");
		return -1;
	}
	memcpy(p->r+2, temp+2, 2);
	release_wordref(p, addr);
	return 0;
}

int load_high(struct proc *p, int addr)
{
char *temp;

	temp = get_wordref(p, addr);
	if (temp == NULL) {
		fprintf(stderr, "load_all: failed to get memory ref\n");
		return -1;
	}
	memcpy(p->r, temp, 2);
	release_wordref(p, addr);
	return 0;
}
