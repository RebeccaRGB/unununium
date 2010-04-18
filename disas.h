// Copyright 2008,2009  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#ifndef _DISAS_H
#define _DISAS_H

#include "types.h"

u32 disas(u32 offset, u16 insn1, u16 insn2);

#endif
