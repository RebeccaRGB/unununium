// Copyright 2008  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include <stdio.h>
#include <string.h>

#include "types.h"
#include "sdl.h"
#include "emu.h"


int main(int argc, char *argv[])
{
	u32 i, n;

	n = fread(mem, 2, N_MEM, stdin);

// gross, but whatever.  one day i'll fix this, but not today
#ifdef _BIG_ENDIAN
	for (i = 0; i < n; i++)
		mem[i] = (mem[i] << 8) | (mem[i] >> 8);
#endif

	sdl_init();

srandom(42);

	emu();

	return 0;
}
