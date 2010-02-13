// Copyright 2008,2009  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#ifndef _IO_H
#define _IO_H

#include "types.h"

void io_store(u16 val, u32 addr);
u16 io_load(u32 addr);

void io_init(void);

#endif
