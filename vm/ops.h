struct proc;
int add(struct proc *p, int addr);
int subtract(struct proc *p, int addr);
int multiply(struct proc *p, int addr);
int divide(struct proc *p, int addr);

int load_all(struct proc *p, int addr);
int load_low(struct proc *p, int addr);
int load_high(struct proc *p, int addr);

int store_register(struct proc *p, int addr);

int set_sp(struct proc *p, int addr);
int get_sp(struct proc *p, int addr);
int push(struct proc *p, int addr);
int pop(struct proc *p, int addr);

int cmp_less(struct proc *p, int addr);
int cmp_eql(struct proc *p, int addr);

int print(struct proc *p, int addr);
int read(struct proc *p, int addr);

int add_stack(struct proc *p, int addr);
int subtract_stack(struct proc *p, int addr);
int multiply_stack(struct proc *p, int addr);
int divide_stack(struct proc *p, int addr);

int recv(struct proc *p, int addr);
int send(struct proc *p, int addr);

int set_sem(struct proc *p, int addr);
int sem_up(struct proc *p, int addr);
int sem_down(struct proc *p, int addr);

int load_shared(struct proc *p, int addr);
int store_shared(struct proc *p, int addr);

int fork_proc(struct proc *p, int addr);
int exec_proc(struct proc *p, int addr);

int jmp(struct proc *p, int ic);
int jmp_if(struct proc *p, int ic);

int nop(struct proc *p, int addr);

int identify(struct proc *p, int addr);

int halt(struct proc *p, int addr);
