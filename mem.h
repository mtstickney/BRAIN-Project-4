/* Include list:
	stdio.h
*/

#define PROCS 100
#define MEMSIZE 100
#define SHMSIZE 100

struct proc;
void init_mem();
char *get_wordref(struct proc *p, int addr);
void release_wordref(struct proc *p, int addr);
int load_shm(int addr, char *dest);
int store_shm(char *src, int addr);
int memalloc(int size);
void freemem(unsigned int addr, unsigned int size);
int dup_mem(struct proc *p, struct proc *q);
int resize_proc(struct proc *p, int newsize);
void print_mem(struct proc *p);
void print_word(FILE* fh, char *word);
void set_mem(char a);
int get_global_address(struct proc *p, int addr);
