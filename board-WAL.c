// Copyright 2008  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include "types.h"
#include "platform.h"

#include "board.h"
//	#include <stdio.h>

static u16 board_WAL_gpio(u32 n, u16 what, u16 push, u16 pull)
{
//	fprintf(stderr, "--->  PORT %c  what=%04x push=%04x pull=%04x\n",
//	        'A' + n, what, push, pull);

	if (n == 0) {
		if (button_up)
			what |= 0x8000;
		if (button_down)
			what |= 0x4000;
		if (button_left)
			what |= 0x2000;
		if (button_right)
			what |= 0x1000;
		if (button_A)
			what |= 0x0800;
		if (button_B)
			what |= 0x0400;
		if (button_menu)
			what |= 0x0020;
	}

	return what;
}

struct board board_WAL = {
	.idle_pc = 0xb1c6,

	.do_gpio = board_WAL_gpio
};
