// Copyright 2008  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#ifndef _EMU_H
#define _EMU_H

#include "types.h"

#define N_MEM 0x400000

extern u16 all_the_mem[4*N_MEM];
extern u16 mem[N_MEM];

void emu(void);

#endif
