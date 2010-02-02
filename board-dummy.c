// Copyright 2008-2010  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include <stdio.h>

#include "types.h"
#include "emu.h"
#include "platform.h"

#include "board.h"


static u16 gpio(u32 n, u16 what, u16 push, u16 pull, u16 special)
{
	return what;
}

struct board board_dummy = {
//	.init = init,
	.gpio = gpio
};
