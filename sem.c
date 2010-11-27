#include "sem.h"
#include <stdio.h>

static unsigned int sem_table[100];

void sem_init()
{
	int i;

	for (i=0; i<100; i++) {
		sem_table[i] = 1;
	}
}

int sem_set(unsigned int sem, unsigned int val)
{
	if (sem > 99)
		return 1;
	sem_table[sem] = val;
	return 0;
}

/* return 0: semaphor down *
 * return -1: error (invalid sem) *
 * return 1: block */
int sem_p(unsigned int sem)
{
	if (sem > 99)
		return -1;
	if (sem_table[sem] == 0)
		return 1;
	sem_table[sem]--;
	return 0;
}

int sem_v(unsigned int sem)
{
	if (sem > 99)
		return 1;
	sem_table[sem]++;
	return 0;
}
