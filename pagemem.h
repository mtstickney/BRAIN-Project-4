struct proc;
int pagemem_init(unsigned int pg_count, unsigned int pg_size, double history_weight);
int touch_page(unsigned int vaddr);
void page_release_ref(int addr);
int page_store(struct proc *p, const char *src, int addr);
void print_pagemem();
