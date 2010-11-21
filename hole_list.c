#include <stdlib.h>
#include <stdio.h>
#include "hole_list.h"

struct hole_list {
	unsigned int addr;
	unsigned int size;
	struct hole_list *next;
};

static struct hole_list *holes=NULL;

int add_hole(unsigned int loc, unsigned int size)
{
	struct hole_list *p;
	struct hole_list *newp;

	fprintf(stderr, "mem: freeing %u words of memory at location %u\n", size, loc);
	if (holes == NULL) {
		newp = malloc(sizeof(struct hole_list));
		if (newp == NULL)
			return 1;
		newp->addr = loc;
		newp->size = size;
		newp->next = NULL;
		holes = newp;
		return 0;
	}
	if (holes->addr >= loc) {
		newp = malloc(sizeof(struct hole_list));
		if (newp == NULL)
			return 1;
		newp->addr = loc;
		newp->size = size;
		newp->next = holes;
		holes = newp;
		return 0;
	}

	for (p=holes; p->next != NULL && p->next->addr < loc; p=p->next);

	newp = malloc(sizeof(struct hole_list));
	if (newp == NULL)
		return 1;
	newp->addr = loc;
	newp->size = size;
	newp->next = p->next;
	p->next = newp;
	return 0;
}

int get_first_hole(unsigned int size)
{
	struct hole_list *p;
	struct hole_list *nextp;
	unsigned int addr;

	if (holes == NULL)
		return -1;
	if (holes->size == size) {
		nextp = holes;
		holes = holes->next;
		addr = nextp->addr;
		free(nextp);
		fprintf(stderr, "mem: allocating %u words of memory at location %u\n", size, addr);
		return addr;
	}
	if (holes->size > size) {
		addr = holes->addr;
		holes->addr += size;
		fprintf(stderr, "mem: allocating %u words of memory at location %u\n", size, addr);
		return addr;
	}

	for (p=holes; p->next!=NULL; p=p->next) {
		if (p->next->size == size) {
			nextp = p->next;
			p->next = p->next->next;
			addr = nextp->addr;
			free(nextp);
			fprintf(stderr, "mem: allocating %u words of memory at location %u\n", size, addr);
			return addr;
		}
		if (p->next->size > size) {
			addr = p->next->addr;
			p->next->addr += size;
			fprintf(stderr, "mem: allocating %u words of memory at location %u\n", size, addr);
			return addr;
		}
	}
	return -1;
}

int get_best_hole(unsigned int size)
{
	struct hole_list *min_parent, *min_hole;
	struct hole_list *p, *nextp;
	unsigned int addr;

	min_hole = NULL;
	for (p=holes; p!=NULL; p=p->next) {
		if (p->size >= size && (min_hole == NULL || p->size < min_hole->size))
			min_hole = p;
	}
	if (min_hole == NULL)
		return -1;

	/* find the parent */
	min_parent = NULL;
	for (p=holes; p!=NULL; p=p->next) {
		if (p->next == min_hole) {
			min_parent = p;
			break;
		}
	}

	if (min_hole->size == size) {
		if (min_parent == NULL) {
			/* first element is the best fit */
			nextp = holes;
			holes = holes->next;
			addr = nextp->addr;
			free(nextp);
			fprintf(stderr, "mem: allocating %u words of memory at location %u\n", size, addr);
			return addr;
		}	
		nextp = min_hole;
		min_parent->next = min_parent->next->next;
		addr = nextp->addr;
		free(nextp);
		fprintf(stderr, "mem: allocating %u words of memory at location %u\n", size, addr);
		return addr;
	}
	if (min_hole->size > size) {
		addr = min_hole->addr;
		min_hole->addr += size;
		fprintf(stderr, "mem: allocating %u words of memory at location %u\n", size, addr);
		return addr;
	}
	return -1; /* shouldn't get here */
}
