#include <stdio.h>
#include <stdlib.h>

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

u32 offset = 0;
u8 op0, opA, op1, opN, opB, opimm;
u16 op, ximm;


const char *regs[] = { "sp", "r1", "r2", "r3", "r4", "bp", "sr", "pc" };

const char *jumps[] = {
	"jb", "jae", "jge", "jl", "jne", "je", "jpl", "jmi",
	"jbe", "ja", "jle", "jg", "jvc", "jvs", "jmp"
};

const char *alu_ops[] = {
	"%s += %s",
	"%s += %s, carry",
	"%s -= %s",
	"%s -= %s, carry",
	"cmp %s, %s",
	"???",
	"%s =- %s",
	"???",
	"%s ^= %s",
	"%s = %s",
	"%s |= %s",
	"%s &= %s",
	"test %s, %s",
	"%2$s = %1$s"
};

static u16 get16(void)
{
	u16 x16;
	int x = getchar();
	if (x < 0)
		exit(0);
	x16 = x;
	x = getchar();
	if (x < 0)
		exit(0);
	x16 |= (x << 8);
	return x16;
}

static const char *b_op(void)
{
	u8 opN = opimm >> 3;
	u8 opB = opimm & 7;
	static char s[80], s2[80];
	const char *forms[] = { "%s", "%s--", "%s++", "++%s" };

	switch (op1) {
	case 0:
		sprintf(s, "[BP+%02x]", opimm);
		break;
	case 1:
		sprintf(s, "%02x", opimm);
		break;
	case 3:
		sprintf(s2, forms[opN & 3], regs[opB]);
		sprintf(s, "%s[%s]", opN & 4 ? "D:" : "", s2);
		break;
	case 4:
		switch(opN) {
		case 0:
			sprintf(s, "%s", regs[opB]);
			break;
		case 1:
			if (opA == opB)
				sprintf(s, "%04x", ximm);
			else
				goto bad;
			break;
		case 2:
			if (opA == opB)
				sprintf(s, "[%04x]", ximm);
			else
				goto bad;
			break;
//		case 3:
//			if (opA == opB)
//				sprintf(s, "%04x", ximm);
//			else
//				goto bad;
//			break;
		case 4: case 5: case 6: case 7:
			sprintf(s, "%s asr %x", regs[opB], (opN & 3) + 1);
			break;
		default:
			goto bad;
		}
		break;
	case 5:
		sprintf(s, "%s ls%c %x", regs[opB], (opN & 4) ? 'r' : 'l',
		        (opN & 3) + 1);
		break;
	case 6:
		sprintf(s, "%s ro%c %x", regs[opB], (opN & 4) ? 'r' : 'l',
		        (opN & 3) + 1);
		break;
	case 7:
		sprintf(s, "[%02x]", opimm);
		break;
	default:
bad:
		sprintf(s, "[[[%x %x %x]]]", op1, opN, opB);
	}

	return s;
}

static void one_insn(void)
{
	op = get16();

	printf("%04x: ", offset);

	op0 = (op >> 12);
	opA = (op >> 9) & 7;
	op1 = (op >> 6) & 7;
	opN = (op >> 3) & 7;
	opB = op & 7;
	opimm = op & 63;


	if ((op1 == 4 && (opN == 1 || opN == 2 || opN == 3))
	 || (op0 == 15 && (op1 == 1 || op1 == 2))) {
		ximm = get16();
		printf("%04x %04x   ", op, ximm);
		offset++;
	} else
		printf("%04x        ", op);
	offset++;


	// first, check for the branch insns
	if (op0 < 15 && opA == 7 && op1 == 0) {
		printf("%s %04x\n", jumps[op0], offset+opimm);
		return;
	}
	if (op0 < 15 && opA == 7 && op1 == 1) {
		printf("%s %04x\n", jumps[op0], offset-opimm);
		return;
	}

	// push/pop insns
	if ((op0 == 9 || op0 == 13) && op1 == 2) {
		if (op0 == 9) {
			if (op == 0x9a90)
				printf("retf\n");
			else if (op == 0x9a98)
				printf("reti\n");
			else if (opA+1 < 8 && opA+opN < 8)
				printf("pop %s, %s from [%s]\n",
				       regs[opA+1], regs[opA+opN], regs[opB]);
			else
				printf("!!! POP\n");
		} else {
			if (opA+1 >= opN && opA < opN+7)
				printf("push %s, %s to [%s]\n",
				       regs[opA+1-opN], regs[opA], regs[opB]);
			else
				printf("!!! PUSH\n");
		}
		return;
	}

	// alu insns
	if (op0 < 14 && op0 != 5 && op0 != 7) {
		printf(alu_ops[op0], regs[opA], b_op());
		printf("\n");
		return;
	}




	printf("???\n");
}

int main(void)
{
	for (;;)
		one_insn();
}
