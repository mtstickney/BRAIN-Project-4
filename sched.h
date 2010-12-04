extern unsigned int ops;

int sched_init();
int sched_suspend(unsigned int pid);
int sched_resume(unsigned int pid);
int sched_reset(unsigned int pid);
int sched_run();

