#include <stdio.h>
#include <string.h>
#include "vm.h"
#include "mem.h"
#include "ops.h"

extern void read_word(char *buf);

int read(struct proc *p, int addr)
{
	char temp[5];
	int i;

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

int print(struct proc *p, int addr)
{
	char temp[4];
	int i;

	for (i=0; i<10; i++) {
		if (load(p, addr++, temp) == -1) {
			fprintf(stderr, "print: load failed\n");
			return -1;
		}
		printf("%c%c%c%c\n", temp[0], temp[1], temp[2], temp[3]);
	}
	return 0;
}
