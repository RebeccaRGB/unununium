// Copyright 2008  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include "types.h"
#include "emu.h"
#include "platform.h"

#include "board.h"

struct board *board;

static struct board *board_detect(void)
{
	if (mem[0x19792] == 0x4311 && mem[0x19794] == 0x4e43)	// VII
		return &board_VII;
	if (mem[0x42daa] == 0x4311 && mem[0x42dac] == 0x4e43)	// VC1
		return &board_VII;
	if (mem[0x3ecb9] == 0x4311 && mem[0x3ecbb] == 0x4e43)	// VC2
		return &board_VII;

	if (mem[0xb1c6] == 0x9311 && mem[0xb1c8] == 0x4501 &&
	    mem[0xb1c9] == 0x5e44)
		return &board_WAL;

	return 0;
}

void board_init(void)
{
	board = board_detect();
	if (board == 0)
		fatal("couldn't detect board\n");
}
