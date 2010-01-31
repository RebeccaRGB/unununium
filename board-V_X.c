// Copyright 2010  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include <stdio.h>

#include "types.h"
#include "emu.h"
#include "platform.h"

#include "board.h"


static void init(void)
{
	if (mem[0x5675c] == 0x9311 && mem[0x5675e] == 0x4240 &&	// Winnie
	    mem[0x5675f] == 0x4e44)
		board->idle_pc = 0x5675c;

	else
		board->idle_pc = 0x4003a;	// studio, FIXME
}

static u16 gpio(u32 n, u16 what, u16 push, u16 pull)
{
	return what;
}

struct board board_V_X = {
	.use_centered_coors = 1,

	.init = init,
	.gpio = gpio
};
