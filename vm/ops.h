struct proc;
int add(struct proc *p, int addr);
int subtract(struct proc *p, int addr);
int multiply(struct proc *p, int addr);
int divide(struct proc *p, int addr);

int load_all(struct proc *p, int addr);
int load_low(struct proc *p, int addr);
int load_high(struct proc *p, int addr);