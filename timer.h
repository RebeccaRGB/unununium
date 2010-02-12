// Copyright 2010  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#ifndef _TIMER_H
#define _TIMER_H

#include "types.h"

extern volatile u32 timer_triggered;

void timer_init(void);
void timer_set(void);

#endif
