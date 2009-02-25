// Copyright 2008,2009  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include "types.h"
#include "board.h"

static void board_VII_init(void)
{
	switch_bank(0);
}

struct board board_VII = {
	.use_centered_coors = 1,
	.init = board_VII_init
};
