// Copyright 2010  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include <stdio.h>

#include "types.h"

#include "timer.h"


static int trace_timer = 0;

static struct timer *timers;

void timer_debug(void)
{
	struct timer *timer;
	for (timer = timers; timer; timer = timer->next)
		printf("timer \"%s\" %u ticks\n", timer->name, timer->time);
}

void timer_add(struct timer *timer)
{
	if (timer->time == 0)
		timer->time = timer->interval;

	u32 time = timer->time;
	struct timer **p = &timers;

	while (*p && (*p)->time <= time) {
		time -= (*p)->time;
		p = &(*p)->next;
	}

	timer->next = *p;
	*p = timer;
	timer->time = time;
	if (timer->next)
		timer->next->time -= time;
}

void timer_run(u32 ticks)
{
//debug("going to run for %u ticks...\n", ticks);
//timer_debug();

	struct timer *timer;
	while ((timer = timers) && timer->time <= ticks) {
		timers = timer->next;
		ticks -= timer->time;

		if (trace_timer)
			printf("running timer \"%s\"\n", timer->name);

		if (timer->run)
			timer->run();

		if (timer->interval) {
			timer->time = timer->interval;
			timer_add(timer);
		}
	}

	if (timer)
		timer->time -= ticks;

//debug("timers done:\n");
//timer_debug();
}
