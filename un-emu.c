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
	disas(mem, ((reg[6] & 0x3f) << 16) | reg[7]);
}

static void step(void)
{
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
	}

	return 0;
}
