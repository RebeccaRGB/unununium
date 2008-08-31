#include <stdio.h>
#include <stdlib.h>

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

static u32 offset = 0;
static u8 op0, opA, op1, opN, opB, opimm;
static u16 op, ximm;


static const char *regs[] = { "sp", "r1", "r2", "r3", "r4", "bp", "sr", "pc" };

static const char *jumps[] = {
	"jb", "jae", "jge", "jl", "jne", "je", "jpl", "jmi",
	"jbe", "ja", "jle", "jg", "jvc", "jvs", "jmp"
};

static const char *alu_ops[] = {
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

static const char *alu_ops_3[] = {
	"%s + %s",
	"%s + %s, carry",
	"%s - %s",
	"%s - %s, carry",
	"???",
	"???",
	"???",
	"???",
	"%s ^ %s",
	"???",
	"%s | %s",
	"%s & %s",
	"???",
	"???"
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
	static char s[80], s2[80];
	const char *forms[] = { "%s", "%s--", "%s++", "++%s" };

	switch (op1) {
	case 0:
		printf("!! WHOOPS !!");
		break;
	case 1:
		if (op0 == 13)
			goto bad;
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
			if (opA == opB && op0 != 13)
				sprintf(s, "%04x", ximm);
			else
				goto bad;
			break;
		case 2:
			if (opA == opB && op0 != 13)
				sprintf(s, "[%04x]", ximm);
			else
				goto bad;
			break;
		case 3:
			if (op0 == 13)
				sprintf(s, "[%04x]", ximm);
			else
				goto bad;
			break;
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


	// all-zero and all-one are invalid insns:
	if (op == 0 || op == 0xffff) {
		printf("--\n");
		return;
	}


	// the top four bits are the alu op or the branch condition, or E or F
	op0 = (op >> 12);

	// the next three are usually the destination register
	opA = (op >> 9) & 7;

	// and the next three the addressing mode
	op1 = (op >> 6) & 7;

	// the next three can be anything
	opN = (op >> 3) & 7;

	// and the last three usually the second register (source register)
	opB = op & 7;

	// the last six sometimes are a single immediate number
	opimm = op & 63;


	// some insns need a second word:
	if ((op1 == 4 && (opN == 1 || opN == 2 || opN == 3))
	 || (op0 == 15 && (op1 == 1 || op1 == 2))) {
		ximm = get16();
		printf("%04x %04x   ", op, ximm);
		offset++;
	} else
		printf("%04x        ", op);

	printf("%x %x %x %x %x   ", op0, opA, op1, opN, opB);

	offset++;


	// first, check for the conditional branch insns
	if (op0 < 15 && opA == 7 && op1 == 0) {
		printf("%s %04x\n", jumps[op0], offset+opimm);
		return;
	}
	if (op0 < 15 && opA == 7 && op1 == 1) {
		printf("%s %04x\n", jumps[op0], offset-opimm);
		return;
	}


	switch ((op1 << 4) | op0) {

	// pop insns
	case 0x29:
		if (op == 0x9a90)
			printf("retf\n");
		else if (op == 0x9a98)
			printf("reti\n");
		else if (opA+1 < 8 && opA+opN < 8)
			printf("pop %s, %s from [%s]\n",
			       regs[opA+1], regs[opA+opN], regs[opB]);
		else
			printf("BAD POP\n");
		return;


	// push insns
	case 0x2d:
		if (opA+1 >= opN && opA < opN+7)
			printf("push %s, %s to [%s]\n",
			       regs[opA+1-opN], regs[opA], regs[opB]);
		else
			printf("!!! PUSH\n");
		return;


	// alu, base+displacement
	case 0x00:
		printf("%s += [bp+%02x]\n", regs[opA], opimm);
		return;
	case 0x01:
		printf("%s += [bp+%02x], carry\n", regs[opA], opimm);
		return;
	case 0x02:
		printf("%s -= [bp+%02x]\n", regs[opA], opimm);
		return;
	case 0x03:
		printf("%s -= [bp+%02x], carry\n", regs[opA], opimm);
		return;
	case 0x04:
		printf("cmp %s, [bp+%02x]\n", regs[opA], opimm);
		return;
	case 0x06:
		printf("%s = -[bp+%02x]\n", regs[opA], opimm);
		return;
	case 0x08:
		printf("%s ^= [bp+%02x]\n", regs[opA], opimm);
		return;
	case 0x09:
		printf("%s = [bp+%02x]\n", regs[opA], opimm);
		return;
	case 0x0a:
		printf("%s |= [bp+%02x]\n", regs[opA], opimm);
		return;
	case 0x0b:
		printf("%s &= [bp+%02x]\n", regs[opA], opimm);
		return;
	case 0x0c:
		printf("test %s, [bp+%02x]\n", regs[opA], opimm);
		return;
	case 0x0d:
		printf("[bp+%02x] = %s\n", opimm, regs[opA]);
		return;


	default:
		// alu insns
		if (op0 < 13 && op0 != 5 && op0 != 7) {
			if (op1 == 4 && (opN == 1 || opN == 2)) {
	printf("XXX   ");
				if (opA == opB)
					printf(alu_ops[op0], regs[opA], b_op());
				else {
	printf("%s = %s %s %04x\n", regs[opA], regs[opB], alu_ops_3[op0], ximm);
	//				printf("--->  %s = ", regs[opA]);
	//				printf(alu_ops_3[op0], opB, ximm);
				}
				printf("\n");
			} else if (op1 == 4 && opN == 3) {
	printf("XXX3   %s = %s %s %04x\n", regs[opA], regs[opB], alu_ops_3[op0], ximm);
	//			printf("--->  %s = ", regs[opA]);
	//			printf(alu_ops_3[op0], opB, ximm);
	//			printf("\n");
			} else {
				printf(alu_ops[op0], regs[opA], b_op());
				printf("\n");
			}
			return;
		}


		// store insns
		if (op0 == 13) {
			if (op1 == 2) {		// push insn
				if (opA+1 >= opN && opA < opN+7)
					printf("push %s, %s to [%s]\n",
					       regs[opA+1-opN], regs[opA], regs[opB]);
				else
					printf("!!! PUSH\n");
			} else {
				printf("STORE: CHECKME!!!   ");
				printf(alu_ops[op0], regs[opA], b_op());
				printf("\n");
			}
			return;
		}

		// E insns
		// ...

		// F insns
		if (op0 == 15) {
			if (opA == 0 && op1 == 1) {
				printf("call %04x\n", (opimm << 16) | ximm);
				return;
			}
		}

		printf("???\n");
	}
}

int main(void)
{
	for (;;)
		one_insn();
}
