// Copyright 2008,2009  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include <stdio.h>

#include "types.h"
#include "emu.h"

#include "audio.h"


int mute_audio;


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

u16 audio_load(u32 addr)
{
	u16 val = mem[addr];

	if (addr >= 0x3000 && addr < 0x3200) {		// audio something
		//printf("LOAD %04x from %04x\n", val, addr);
		return val;
	}
	if (addr >= 0x3200 && addr < 0x3300) {		// audio something
		//printf("LOAD %04x from %04x\n", val, addr);
		return val;
	}
	if (addr >= 0x3400 && addr < 0x3500) {		// audio something
		//printf("LOAD %04x from %04x\n", val, addr);
		return val;
	}

	printf("UNKNOWN AUDIO LOAD from %04x\n", addr);
	return val;
}

void audio_init(void)
{
}
