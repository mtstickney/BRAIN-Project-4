#include <stdio.h>
#include <string.h>
#include "mem.h"
#include "vm.h"
#include "hole_list.h"

static char mem[4*100*PROCS];
static char shm[4*100];

void set_mem(char a) {
	memset(mem, a, 4*100*PROCS);
}

int memalloc(int size)
{
	if (size < 0) {
		fprintf(stderr, "memalloc: invalid size\n");
		return -1;
	}
	return get_first_hole(size);
}

void freemem(unsigned int addr, unsigned int size)
{
	add_hole(addr, size);
}

void init_mem()
{
	add_hole(0, 100*PROCS);
}

extern int word2int(char *p);
extern void int2word(int a, char *b);
static char *get_memp(struct proc *p, int addr) {
	int limit,base;

	limit = word2int(p->lr);
	base = word2int(p->br);
	if (limit < 0 || base < 0) {
		fprintf(stderr, "get_memp: invalid base or limit register\n");
		return NULL;
	}
	if (addr > limit || addr < 0) {
		fprintf(stderr, "get_memp: invalid address\n");
		return NULL;
	}
	return &mem[4*(base+addr)];
}

/* Note: load and store convert endianness of words. */
int load(struct proc *p, int addr, char *dest) {
	char *src;

	src = get_memp(p, addr);
	if (src == NULL) {
		fprintf(stderr, "load: invalid address %d\n", addr);
		return -1;
	}
	memcpy(dest, src, 4);
	return 0;
}

int store(struct proc *p, const char *src, int addr)
{
	char *dest;

	dest = get_memp(p, addr);
	if (dest == NULL) {
		fprintf(stderr, "store: invalid address %d\n", addr);
		return -1;
	}

	memcpy(dest, src, 4);
	return 0;
}

static char *get_shmp(int addr) {
	if (addr > 99 || addr < 0) {
		fprintf(stderr, "get_shmp: invalid address %u\n", addr);
		return NULL;
	}
	return &shm[4*addr];
}

int load_shm(int addr, char *dest)
{
	char *src;

	src = get_shmp(addr);
	if (src == NULL) {
		fprintf(stderr, "load_shm: invalid address %u\n", addr);
		return 1;
	}
	memcpy(dest, src, 4);
	return 0;
}

int store_shm(char *src, int addr)
{
	char *dest;

	dest = get_shmp(addr);
	if (dest == NULL) {
		fprintf(stderr, "store_shm: invalid address %u\n", addr);
		return 1;
	}
	memcpy(dest, src, 4);
	return 0;
}

/* make process q's address space equivalent to process p's */
int dup_mem(struct proc *p, struct proc *q)
{
	int i,j;
	int c;
	char *src, *dest;

	i = word2int(p->lr);
	j = word2int(q->lr);
	for (c=0; c<=i && c<=j; c++) {
		src = get_memp(p, c);
		dest = get_memp(q, c);
		if (src == NULL || dest == NULL) {
			fprintf(stderr, "dup_mem: get_memp failed\n");
			return -1;
		}
		memcpy(dest, src, 4);
	}
	return 0;
}

int resize_proc(struct proc *p, int newsize)
{
	int addr;
	int base, limit;

	addr = memalloc(newsize);
	if (addr < 0)
		return -1;
	base = word2int(p->br);
	limit = word2int(p->lr);
	if (base < 0 || limit < 0) {
		fprintf(stderr, "realloc_proc: warning: invalid base or limit register, not freeing memory");
	} else {
		freemem(base, limit+1);
	}
	int2word(addr, p->br);
	int2word(newsize-1, p->lr);
	return 0;
}
	

void print_word(FILE* fh, char *word)
{
	fprintf(fh, "%c%c%c%c", word[0], word[1], word[2], word[3]);
}

void print_mem()
{
	int i,j;
	char *p;

	for (i=0; i<1000; i++) {
		printf("%03d ", i);
		for (j=0; j<9; j++, i++) {
			p = &mem[i*4];
			printf("%c%c%c%c ", p[0], p[1], p[2], p[3]);
		}
		p=&mem[i*4];
		printf("%c%c%c%c\n", p[0], p[1], p[2], p[3]);
	}
}
