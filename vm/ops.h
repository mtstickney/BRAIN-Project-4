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