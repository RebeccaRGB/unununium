// Copyright 2008-2010  Segher Boessenkool  <segher@kernel.crashing.org>
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

	if (mem[0x5ce1] == 0x42c2 && mem[0x5ce2] == 0x5e42)
		return &board_BAT;

	if (mem[0x5675c] == 0x9311 && mem[0x5675e] == 0x4240 &&	// Winnie
	    mem[0x5675f] == 0x4e44)
		return &board_V_X;

return &board_V_X;

	return 0;
}

void board_init(void)
{
	board = board_detect();
	if (board == 0) {
		warn("couldn't detect board\n");
		board = &board_dummy;
	}

	if (board->init)
		board->init();
}
