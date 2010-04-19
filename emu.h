// Copyright 2008-2010  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#ifndef _EMU_H
#define _EMU_H

#include "types.h"

struct memory_chip {
	u16 *data;
	u32 mask;
	u32 extra;
};

extern struct memory_chip main_rom;
extern struct memory_chip *memory_chips[4];

extern u16 ram[0x4000];

u16 load(u32 addr);
void store(u16 val, u32 addr);

void emu(void);

u32 get_ds(void);
void set_ds(u32 ds);
void set_address_decode(u32 x);
u16 get_video_line(void);

#endif
