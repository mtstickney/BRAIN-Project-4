/* Include list:
	stdio.h
*/

#define PROCS 100

struct proc;
void init_mem();
int load(struct proc *p, int addr, char *dest);
int store(struct proc *p, const char *src, int addr);
int load_shm(int addr, char *dest);
int store_shm(char *src, int addr);
int memalloc(int size);
void freemem(unsigned int addr, unsigned int size);
int dup_mem(struct proc *p, struct proc *q);
int resize_proc(struct proc *p, int newsize);
void print_mem();
void print_word(FILE* fh, char *word);
void set_mem(char a);
