// Copyright 2008  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#ifndef _EMU_H
#define _EMU_H

#define N_MEM 0x400000

extern u16 mem[N_MEM];
extern u16 reg[8];

void emu(void);

#endif
