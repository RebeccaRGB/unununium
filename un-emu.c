// Copyright 2008  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "disas.h"


#define N_MEM 0x400000


static u16 mem[N_MEM];
static u16 reg[8];
static u64 insn_count = 0;


static u32 cs_pc(void)
{
	return ((reg[6] & 0x3f) << 16) | reg[7];
}

static void print_state(void)
{
	int i;

	printf("\n");
	printf(" SP   R1   R2   R3   R4   BP   SR   PC   CS NZSC DS\n");
	for (i = 0; i < 8; i++)
		printf("%04x ", reg[i]);
	printf(" %02x", reg[6] & 0x3f);
	printf(" %x%x%x%x", (reg[6] >> 6) & 1, (reg[6] >> 7) & 1,
	                    (reg[6] >> 8) & 1, (reg[6] >> 9) & 1);
	printf(" %02x\n", reg[6] >> 10);
	disas(mem, cs_pc());
}

static void step(void)
{
	u16 op;
	u8 op0, opA, op1, opN, opB, opimm;
	u16 x, x0, x1;
	u32 old_cs_pc = cs_pc();
	u32 d;

	op = mem[cs_pc()];
	reg[7]++;


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


	// jumps
	if (opA == 7 && op1 < 2) {
		switch (op0) {
		case 14:
			goto do_jump;
		default:
			goto bad;

		do_jump:
			if (op1 == 0)
				reg[7] += opimm;
			else
				reg[7] -= opimm;
			return;
		}
	}


	// first, get the arguments
	x0 = reg[opA];

	switch (op1) {
	case 3:
		if ((opN & 3) == 3)
			reg[opB]++;
		d = reg[opB];
		if (opN & 4)
			d |= (reg[6] << 6) & 0x3f0000;
		x1 = mem[d];
		if ((opN & 3) == 1)
			reg[opB]--;
		if ((opN & 3) == 2)
			reg[opB]++;
		break;
	case 4:
		switch(opN) {
		case 1:
			x0 = reg[opB];
			x1 = mem[cs_pc()];
			reg[7]++;
			break;
		default:
			goto bad;
		}
		break;
	default:
		goto bad;
	}

printf("--> args: %04x %04x\n", x0, x1);

	// then, perform the alu op
	switch (op0) {
	case 9:
		x = x1;
		// XXX: flags
		break;
	default:
		goto bad;
	}

printf("--> result: %04x\n", x);

	reg[opA] = x;
	return;


bad:
	printf("\n[insn_count = %llu]\n", insn_count);
	printf("UNIMPLEMENTED:\n");
	disas(mem, old_cs_pc);
	exit(1);
}

int main(void)
{
	u32 i, n;

	n = fread(mem, 2, N_MEM, stdin);

	for (i = 0; i < n; i++)
		mem[i] = (mem[i] << 8) | (mem[i] >> 8);

	memset(reg, 0, sizeof reg);
	reg[7] = mem[0xfff7];	// reset vector

	for (;;) {
		print_state();
		step();
		insn_count++;
	}

	return 0;
}
