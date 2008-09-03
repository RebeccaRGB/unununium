// Copyright 2008  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include <stdio.h>
#include <stdlib.h>

#include "types.h"
#include "disas.h"

#define N_MEM 0x400000

static u16 mem[N_MEM];

int main(void)
{
	u32 i, n;

	n = fread(mem, 2, N_MEM, stdin);

	for (i = 0; i < n; i++)
		mem[i] = (mem[i] << 8) | (mem[i] >> 8);

	for (i = 0; i < n; )
		i += disas(mem, i);

	return 0;
}
