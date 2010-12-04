#include <stdio.h>
#include <string.h>
#include "mem.h"
#include "vm.h"
#include "hole_list.h"
#include "pagemem.h"

#define MEMBYTES 4*MEMSIZE
#define SHMBYTES 4*SHMSIZE

char mem[MEMBYTES];
static char shm[SHMBYTES];

void set_mem(char a) {
	memset(mem, a, MEMBYTES);
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
	add_hole(0, MEMSIZE);
}

extern int word2int(char *p);
extern void int2word(int a, char *b);

int get_global_address(struct proc *p, int addr)
{
	int base, limit;
	
	if (addr < 0)
	{
		fprintf(stderr, "get_global_address: invalid address\n");
		return -1;
	}
	base = word2int(p->br);
	limit = word2int(p->lr);
	if (base < 0 || limit < 0) {
		fprintf(stderr, "get_global_address: invalid base or limit register\n");
		return -1;
	}
	if (addr > limit) {
		fprintf(stderr, "get_global_address: address past process limit\n");
		return -1;
	}
	return base+addr;
}

/* Note: load and store convert endianness of words. */
char *get_wordref(struct proc *p, int addr) {
	int global_addr;
	int phys_addr;

	global_addr = get_global_address(p, addr);
	if (global_addr < 0) {
		fprintf(stderr, "get_wordref: invalid global address\n");
		return NULL;
	}
	phys_addr = touch_page(global_addr);
	if (phys_addr < 0) {
		fprintf(stderr, "get_wordref: error bringing page in\n");
		return NULL;
	}
	return &mem[4*phys_addr];
}

void release_wordref(struct proc *p, int addr)
{
	int global_addr;
	
	global_addr = get_global_address(p, addr);
	if (global_addr < 0) {
		fprintf(stderr, "release_ref: invalid global address\n");
		return;
	}
	page_release_ref(global_addr);
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
		src = get_wordref(p, c);
		dest = get_wordref(q, c);
		if (src == NULL || dest == NULL) {
			fprintf(stderr, "dup_mem: get_wordref failed\n");
			return -1;
		}
		memcpy(dest, src, 4);
		release_wordref(p, c);
		release_wordref(q, c);
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


