// Copyright 2010  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#ifndef _TIMER_H
#define _TIMER_H

#include "types.h"

struct timer {
	const char *name;
	u32 time;
	u32 interval;
	void (*run)(void);
	struct timer *next;
};

void timer_debug(void);

void timer_add(struct timer *timer);
void timer_run(u32 ticks);

#endif
