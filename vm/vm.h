#define LEN(a) (sizeof(a)/sizeof((a)[0]))

struct proc
{
	char r[4];
	char sp[4];
	char br[4];
	char lr[4];
	char ic[2];
	char c;
	unsigned int stack_base;
	unsigned int pid;
};

extern struct proc proc_table[100];

#define OPCODE(a, b) (((int)(a)<<8)+(b))

enum OP {
	LR = OPCODE('L', 'R'),
	LL = OPCODE('L', 'L'),
	LH = OPCODE('L', 'H'),
	SR = OPCODE('S', 'R'),
	SP = OPCODE('S', 'P'),
	PS = OPCODE('P', 'S'),
	PH = OPCODE('P', 'H'),
	PP = OPCODE('P', 'P'),
	CE = OPCODE('C', 'E'),
	CL = OPCODE('C', 'L'),
	BT = OPCODE('B', 'T'),
	BU = OPCODE('B', 'U'),
	GD = OPCODE('G', 'D'),
	PD = OPCODE('P', 'D'),
	AD = OPCODE('A', 'D'),
	SU = OPCODE('S', 'U'),
	MI = OPCODE('M', 'U'),
	DI = OPCODE('D', 'I'),
	AS = OPCODE('A', 'S'),
	SS = OPCODE('S', 'S'),
	MS = OPCODE('M', 'S'),
	DS = OPCODE('D', 'S'),
	NP = OPCODE('N', 'P'),
	SD = OPCODE('S', 'D'),
	RC = OPCODE('R', 'C'),
	SI = OPCODE('S', 'I'),
	PE = OPCODE('P', 'E'),
	VE = OPCODE('V', 'E'),
	LS = OPCODE('L', 'S'),
	ST = OPCODE('S', 'T'),
	FK = OPCODE('F', 'K'),
	EX = OPCODE('E', 'X'),
	GP = OPCODE('G', 'P'),
	HA = OPCODE('H', 'A')
};

struct op
{
	enum OP opcode;
	int (*run)(struct proc *p, int addr);
};

int tick(unsigned int pid);
int word2int(char *);
void int2word(int a, char *b);
void msg_init();
void sem_op_init();
int retry_op(struct proc *p);
struct proc *procalloc(unsigned int size);
