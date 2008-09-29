// Copyright 2008  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include <stdio.h>

#include "types.h"
#include "emu.h"
#include "audio.h"


void audio_store(u16 val, u32 addr)
{
	mem[addr] = val;

	if (addr < 0x3200) {		// XXX
		return;
	} else if (addr < 0x3400) {	// XXX
		return;
	} else {			// XXX
		return;
	}

	printf("AUDIO STORE %04x to %04x\n", val, addr);
}
