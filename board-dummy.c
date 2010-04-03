// Copyright 2008-2010  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include <stdio.h>

#include "types.h"
#include "emu.h"
#include "platform.h"

#include "board.h"


static u16 gpio(__unused u32 n, u16 what, __unused u16 push, __unused u16 pull, __unused u16 special)
{
	return what;
}

struct board board_dummy = {
//	.init = init,
	.gpio = gpio
};
