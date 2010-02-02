// Copyright 2008,2009  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#ifndef _BOARD_H
#define _BOARD_H

#include "types.h"

struct board {
	u32 idle_pc;		// Address of code that waits for (retrace) IRQ
	int use_centered_coors;	// This is some video reg actually, FIXME

	void (*init)(void);
	u16 (*gpio)(u32 n, u16 what, u16 push, u16 pull, u16 special);
};

extern struct board board_VII, board_WAL, board_BAT, board_V_X, board_dummy;
extern struct board *board;

void board_init(void);

#endif
