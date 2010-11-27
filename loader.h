/* Include list:
	stdio.h
*/
struct proc;
int load_header(FILE *fh);
int load_code(FILE *fh, struct proc *p);

