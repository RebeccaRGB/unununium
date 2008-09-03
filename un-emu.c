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
static u8 sb;
static u64 insn_count = 0;


static void store(u16 val, u32 addr)
{
	mem[addr] = val;
	if (addr >= 0x2800)
		printf("STORE %04x to %04x\n", val, addr);
}

static void dump(u32 addr, u32 len)
{
	u32 off, i;

	for (off = addr & ~0x0f; off < addr + len; off += 0x10) {
		printf("%06x:", off);

		for (i = 0; i < 16; i++)
			printf(" %04x", mem[off+i]);
		printf("\n");
	}
}

static u32 cs_pc(void)
{
	return ((reg[6] & 0x3f) << 16) | reg[7];
}

static void push(u16 val, u8 b)
{
	store(val, reg[b]--);
}

static u16 pop(u8 b)
{
	return mem[++reg[b]];
}

static void print_state(void)
{
	int i;

	printf("\n[insn_count = %llu]\n", insn_count);
	printf(" SP   R1   R2   R3   R4   BP   SR   PC   CS NZSC DS  SB\n");
	for (i = 0; i < 8; i++)
		printf("%04x ", reg[i]);
	printf(" %02x", reg[6] & 0x3f);
	printf(" %x%x%x%x", (reg[6] >> 9) & 1, (reg[6] >> 8) & 1,
	                    (reg[6] >> 7) & 1, (reg[6] >> 6) & 1);
	printf(" %02x", reg[6] >> 10);
	printf("  %02x\n", sb);
	disas(mem, cs_pc());
}

static void step(void)
{
	u16 op;
	u8 op0, opA, op1, opN, opB, opimm;
	u16 x, x0, x1;
	u32 old_cs_pc = cs_pc();
	u32 d = 0xff0000;

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
		case 0:
			if ((reg[6] & 0x40) == 0)
				goto do_jump;
			break;
		case 2:
			if ((reg[6] & 0x80) == 0)
				goto do_jump;
			break;
		case 4:
			if ((reg[6] & 0x0100) == 0)
				goto do_jump;
			break;
		case 5:
			if ((reg[6] & 0x0100) == 0x0100)
				goto do_jump;
			break;
		case 8:
			if ((reg[6] & 0x0140) != 0x0040)
				goto do_jump;
			break;
		case 9:
			if ((reg[6] & 0x0140) == 0x0040)
				goto do_jump;
			break;
		case 10:
			if ((reg[6] & 0x0180) != 0)
				goto do_jump;
			break;
		case 11:
			if ((reg[6] & 0x0180) == 0)
				goto do_jump;
			break;
		case 14:
			goto do_jump;
		default:
			goto bad;

		do_jump:
			if (op1 == 0)
				reg[7] += opimm;
			else
				reg[7] -= opimm;
		}
		return;
	}


	// push
	if (op1 == 2 && op0 == 13) {
		while (opN--)
			push(reg[opA--], opB);
		return;
	}

	// pop
	if (op1 == 2 && op0 == 9) {
		// XXX: special case for reti
		while (opN--)
			reg[++opA] = pop(opB);
		return;
	}


	if (op0 == 15) {
		switch (op1) {
		case 0:
			if (opN == 1) {
				if (opA == 7)
					goto bad;
				u32 x = reg[opA]*reg[opB];
//printf("MUL US: %08x\n", x);
				if (reg[opB] & 0x8000)
					x -= reg[opA] << 10;
//printf("MUL US: %08x\n", x);
				reg[4] = x >> 16;
				reg[3] = x;
				return;
			} else
				goto bad;
		case 1:		// call
			x1 = mem[cs_pc()];
			reg[7]++;
			push(reg[7], 0);
			push(reg[6], 0);
			reg[7] = x1;
			reg[6] = (reg[6] & 0xffc0) | opimm;
			return;
		case 5:		// irq flags
			printf("IRQ FLAG INSN: %02x\n", opimm);
			return;
		default:
			goto bad;
		}
	}


	// alu op

	// first, get the arguments
	x0 = reg[opA];

	switch (op1) {
	case 0:
		d = reg[5] + opimm;
		x1 = mem[d];
		break;
	case 1:
		x1 = opimm;
		break;
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
		case 0:
			x1 = reg[opB];
			break;
		case 1:
			x0 = reg[opB];
			x1 = mem[cs_pc()];
			reg[7]++;
			break;
		case 2:
			x0 = reg[opB];
			d = mem[cs_pc()];
			x1 = mem[d];
			reg[7]++;
			break;
		case 3:
			x0 = reg[opB];
			x1 = reg[opA];
			d = mem[cs_pc()];
			reg[7]++;
			break;
		default:
			goto bad;
		}
		break;
	case 5:
		if (opN < 4) {
			u32 shift = ((sb << 16) | reg[opB]) << (opN + 1);
			sb = shift >> 16;
			x1 = shift;
		} else {
			u32 shift = ((reg[opB] << 4) | sb) >> (opN - 3);
			sb = shift & 0x0f;
			x1 = shift >> 4;
		}
		break;
	case 6:
		if (opN < 4) {
			goto bad;
		} else {
			u32 shift = ((((sb << 16) | reg[opB]) << 4) | sb) >> (opN - 3);
			sb = shift & 0x0f;
			x1 = shift >> 4;
		}
		break;
	default:
		goto bad;
	}

//printf("--> args: %04x %04x\n", x0, x1);

	// then, perform the alu op
	switch (op0) {
	case 0:
		x = x0 + x1;
		break;
	case 2: case 4:
		x = x0 + ~x1 + 1;
		break;
	case 6:
		x = -x1;
		break;
	case 8:
		x = x0 ^ x1;
		break;
	case 9:
		x = x1;
		break;
	case 10:
		x = x0 | x1;
		break;
	case 11: case 12:
		x = x0 & x1;
		break;
	case 13:
		store(x0, d);
		return;
	default:
		goto bad;
	}

	// set N and Z flags
	if (op0 < 13) {
		reg[6] = (reg[6] & ~0x0300);
		if (x & 0x8000)
			reg[6] |= 0x0200;
		if (x == 0)
			reg[6] |= 0x0100;
	}

	// set S and C flags
	if (op0 < 5) {
		reg[6] = (reg[6] & ~0x0c0);
		if (x < x0)
			reg[6] |= 0x40;
		reg[6] |= ((reg[6] & 0x0100) >> 1) ^ ((reg[6] & 0x40) << 1);
	}

	if (op0 == 4 || op0 == 12)
		return;

//printf("--> result: %04x\n", x);

	if (op1 == 4 && opN == 3) {
		store(x, d);
//printf("4.3 STORE STRANGE: [%04x] = %04x\n", d, x);
	} else
		reg[opA] = x;

	return;


bad:
	reg[7] = old_cs_pc;
	print_state();
	printf("! UNIMPLEMENTED\n");

//	printf("\n\nram dump:\n");
//	dump(0, 0x2800);
	exit(1);
}

int main(void)
{
	u32 i, n;

	n = fread(mem, 2, N_MEM, stdin);

// gross, but whatever
#ifdef _BIG_ENDIAN
	for (i = 0; i < n; i++)
		mem[i] = (mem[i] << 8) | (mem[i] >> 8);
#endif

	memset(reg, 0, sizeof reg);
	reg[7] = mem[0xfff7];	// reset vector

	for (;;) {
		if (insn_count % 1000000 == 0)
			print_state();
		step();
		insn_count++;
	}

	return 0;
}
