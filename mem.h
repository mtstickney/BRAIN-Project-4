/* Include list:
	stdio.h
*/
struct proc;
void init_mem();
int load(struct proc *p, unsigned int addr, char *dest);
int store(struct proc *p, const char *src, unsigned int addr);
int load_shm(unsigned int addr, char *dest);
int store_shm(char *src, unsigned int addr);
int memalloc(unsigned int size);
void freemem(unsigned int addr, unsigned int size);
void print_mem();
void print_word(FILE* fh, char *word);
void set_mem(char a);
