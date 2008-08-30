#include <stdio.h>

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

u32 offset = 0;

char *jumps[] = {
	"jb", "jae", "jge", "jl", "jne", "je", "jpl", "jmi",
	"jbe", "ja", "jle", "jg", "jvc", "jvs", "jmp"
};

static void one_insn(u16 op)
{
	u8 op0, opA, op1, opN, opB, opimm;

	op0 = (op >> 12);
	opA = (op >> 9) & 7;
	op1 = (op >> 6) & 7;
	opN = (op >> 3) & 7;
	opB = op & 7;
	opimm = op & 63;

	printf("%04x: ", offset);
	printf("%04x        ", op);	// XXX: second opcode word

	offset++;

	// first, check for the branch insns
	if (op0 != 15 && opA == 7 && op1 == 0) {
		printf("%s %04x\n", jumps[op0], offset+opimm);
		return;
	}
	if (op0 != 15 && opA == 7 && op1 == 1) {
		printf("%s %04x\n", jumps[op0], offset-opimm);
		return;
	}

	printf("???\n");
}

int main(void)
{
	for (;;) {
		u16 op;
		int x = getchar();
		if (x < 0)
			break;
		op = x;
		x = getchar();
		if (x < 0)
			break;
		op |= (x << 8);
		one_insn(op);
	}

	return 0;
}
