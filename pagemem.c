#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "vm.h"
#include "sched.h"
#include "mem.h"
#include "pagemem.h"

struct pager {
	unsigned int pg_count;
	unsigned int pg_size;
};

struct page_ent {
	int valid;
	unsigned int frame_addr;
	unsigned int refcount;
	/* unsigned int page; pretty sure we aren't using this */
};

extern char mem[MEMSIZE];

static char *page_mem=NULL;
static struct page_ent *pg_table=NULL;
static unsigned int *free_frames=NULL;

static struct pager sys_pager;

struct freq_ent
{
	/* last_access is in operations (ops global from sched.h) */
	unsigned int last_access;
	/* period between page accesses */
	unsigned int period;
};

static struct frequency_table
{
	struct freq_ent *pg_period;
	double history_weight;
} freq_table;

/* the following init functions expect sys_pager.pg_count and */
/* sys_pager.pg_size to be set before being called */
static int init_page_table()
{
	int i;
	
	pg_table = malloc(sys_pager.pg_count*sizeof(struct page_ent));
	if (pg_table == NULL) {
		fprintf(stderr, "init_page_table: alloc failed\n");
		return -1;
	}
	for (i=0; i<sys_pager.pg_count; i++) {
		pg_table[i].valid = 0;
		/* pg_table[i].page = i; FIXME*/
		pg_table[i].frame_addr = 0;
	}
	return 0;
}

static int init_frequency_table(double hist_weight)
{
	int i;
	
	freq_table.history_weight = hist_weight;
	freq_table.pg_period = malloc(sys_pager.pg_count*sizeof(struct freq_ent));
	if (freq_table.pg_period == NULL) {
		fprintf(stderr, "init_frequency_table: alloc failed\n");
		return -1;
	}
	for (i=0; i<sys_pager.pg_count; i++) {
		freq_table.pg_period[i].last_access = 0;
		freq_table.pg_period[i].period = 0;
	}
	return 0;
}

static int init_free_frames()
{
	int i;
	
	free_frames = malloc(sys_pager.pg_count*sizeof(unsigned int));
	if (free_frames == NULL) {
		fprintf(stderr, "init_free_frames: alloc failed\n");
		return -1;
	}
	for (i=0; i<MEMSIZE/sys_pager.pg_size; i++)
		free_frames[i] = 1;
	return 0;
}

/* sizes are in words, not bytes */
int pagemem_init(unsigned int pg_count, unsigned int pg_size, double history_weight)
{
	if (pg_size > MEMSIZE) {
		fprintf(stderr, "pagemem_init: page size is larger than available memory\n");
		return -1;
	}
	sys_pager.pg_count = pg_count;
	sys_pager.pg_size = pg_size;
	/* setup the virtual address space */
	freemem(0, pg_size*pg_count);
	page_mem = malloc(pg_count*pg_size*4);
	memset(page_mem, '0', pg_count*pg_size*4);
	if (page_mem == NULL) {
		fprintf(stderr, "pagemem_init: failed to allocate secondary storage\n");
		goto ERR_RET;
	}
	if (init_page_table() == -1)
		goto ERR_PMEM;
	if (init_frequency_table(history_weight) == -1)
		goto ERR_PG_TABLE;
	if (init_free_frames() == -1)
		goto ERR_FREQ_TABLE;
	return 0;
ERR_FREQ_TABLE:
	free(freq_table.pg_period);
ERR_PG_TABLE:
	free(pg_table);
ERR_PMEM:
	free(page_mem);
ERR_RET:
	return -1;
}

/* get least frequently used page that doesn't still have */
/* references open. Uses an exponential averaging scheme based on */
/* time between accesses. freq_table.history_weight determines how */
/* quickly the frequency responds to changing access patterns (lower */
/* means faster response, higher improves consistency). */
static int get_removal_page()
{
	int i, min_index;
	unsigned int min_period;
	
	min_index = -1;
	for (i=0; i<sys_pager.pg_count; i++) {
		if (pg_table[i].refcount > 0)
			continue;
		if (min_index == -1 || freq_table.pg_period[i].period < min_period) {
			min_index = i;
			min_period = freq_table.pg_period[i].period;
		}
	}
	return min_index;
}

void print_pagemem()
{
	int i,j;
	char *p;

	for (i=0; i<1000; i++) {
		printf("%03d ", i);
		for (j=0; j<9; j++, i++) {
			p = &page_mem[i*4];
			printf("%c%c%c%c ", p[0], p[1], p[2], p[3]);
		}
		p=&page_mem[i*4];
		printf("%c%c%c%c\n", p[0], p[1], p[2], p[3]);
	}
}

static void store_page(int page)
{
	char *framep;
	
	if (page < 0 || page > sys_pager.pg_count)
		return;
	framep = &mem[pg_table[page].frame_addr*4];
	if (framep == NULL) {
		fprintf(stderr, "store_page: warning: failed to get frame pointer\n");
		return;
	}
	memcpy(&page_mem[page*sys_pager.pg_size*4], framep, sys_pager.pg_size*4);
	return;
}

static void update_page_info(int page)
{
	struct freq_ent *freqp;
	
	if (page < 0 || page > sys_pager.pg_count)
	{
		fprintf(stderr, "update_page_info: info refresh on invalid page\n");
		return;
	}
	pg_table[page].refcount++;
	
	freqp = &(freq_table.pg_period[page]);
	freqp->period *= freq_table.history_weight;
	freqp->period += (1-freq_table.history_weight)*(ops-freqp->last_access);
	freqp->last_access = ops;
}

static int toss_page()
{
	int page;
	int frame;
	
	page = get_removal_page();
	if (page < 0) {
		fprintf(stderr, "toss_page: all valid pages in use. Increase page count or size.\n");
		return -1;
	}
	store_page(page);
	pg_table[page].valid = 0;
	frame = pg_table[page].frame_addr / sys_pager.pg_size;
	free_frames[frame] = 1;
	return 0;
}

/* pull a page into the first available free frame */
static int page_in(unsigned int page)
{
	int i;
	int frame_count;
	char *framep;

	frame_count = MEMSIZE/sys_pager.pg_size;
	for (i=0; i<frame_count; i++) {
		if (free_frames[i] != 0)
			break;
	}
	if (i == frame_count) {
		fprintf(stderr, "page_in: no free frames\n");
		return -1;
	}
	pg_table[page].frame_addr = i*sys_pager.pg_size;
	framep = &mem[i*sys_pager.pg_size*4];
	if (framep == NULL) {
		fprintf(stderr, "page_in: failed to get frame pointer\n");
		return -1;
	}
	memcpy(framep, &page_mem[page*sys_pager.pg_size*4], sys_pager.pg_size*4);
	pg_table[page].valid = 1;
	free_frames[i] = 0;
	return 0;
}

/* takes a global virtual address, returns address in new frame */
int touch_page(unsigned int vaddr)
{
	int page, offset;
	int frame_count;
	int i;
	
	page = vaddr / sys_pager.pg_size;
	offset = vaddr % sys_pager.pg_size;
	if (page > sys_pager.pg_count-1) {
		fprintf(stderr, "page_in: address points to nonexistent page\n");
		return -1;
	}
	if (!pg_table[page].valid) {
		fprintf(stderr, "PAGE FAULT\n");
		frame_count = MEMSIZE/sys_pager.pg_size;
		for (i=0; i<frame_count; i++) {
			if (free_frames[i] == 1)
				break;
		}
		if (i == frame_count)
			toss_page();
		if (page_in(page) == -1) {
			fprintf(stderr, "touch_page: error bringing page into mem\n");
			return -1;
		}
	}
	update_page_info(page);
	
	return pg_table[page].frame_addr+offset;
}

void page_release_ref(int addr)
{
	int page;
	
	if (addr < 0) {
		fprintf(stderr, "page_release_ref: invalid address\n");
		return;
	}
	page = addr / sys_pager.pg_size;
	if (page > sys_pager.pg_count-1) {
		fprintf(stderr, "page_release_ref: address points to nonexistant page\n");
		return;
	}
	if (pg_table[page].refcount == 0) {
		fprintf(stderr, "page_release_ref: warning: releasing ref for page with 0 refcount\n");
		return;
	}
	pg_table[page].refcount--;
}

int page_store(struct proc *p, const char *src, int addr)
{
	int global_addr;
	
	if (p == NULL) {
		fprintf(stderr, "page_store: invalid process\n");
		return -1;
	}
	global_addr = get_global_address(p, addr);
	if (global_addr < 0) {
		fprintf(stderr, "page_store: invalid address\n");
		return -1;
	}
	/* secondary storage maps directly to the address space, so just copy it */
	memcpy(&page_mem[global_addr*4], src, 4);
	return 0;
}
