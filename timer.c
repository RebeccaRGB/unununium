// Copyright 2010  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include <sys/time.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>

#include "types.h"
#include "platform.h"

#include "timer.h"


volatile u32 timer_triggered;

static struct timer *timers;
static u32 timer_time;

u32 timer_now(void)
{
	struct timeval tv;

	gettimeofday(&tv, 0);
	return 1000000*tv.tv_sec + tv.tv_usec;
}

void timer_set(u32 usecs)
{
	struct itimerval it;

	it.it_value.tv_sec = usecs / 1000000;
	it.it_value.tv_usec = usecs % 1000000;
	it.it_interval.tv_sec = 0;
	it.it_interval.tv_usec = 0;

	int err = setitimer(ITIMER_REAL, &it, 0);
	if (err)
		fatal("setitimer failed");
}

void timer_debug(void)
{
	printf("timer head = %u, now = %u\n", timer_time, timer_now());
	struct timer *timer;
	for (timer = timers; timer; timer = timer->next)
		printf("timer \"%s\" %dus\n", timer->name, timer->time);
}

void timer_add(struct timer *timer)
{
	u32 time = timer_now() - timer_time + timer->time;
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

void timer_run(void)
{
	u32 now = timer_now();
	u32 elapsed = now - timer_time;

	struct timer *timer;
	while ((timer = timers) && timer->time <= elapsed) {
		timers = timer->next;
		timer_time += timer->time;
		elapsed -= timer->time;

printf("running timer \"%s\" (%u)\n", timer->name, now);
		if (timer->run)
			timer->run();

		if (timer->interval) {
			timer->time = timer->interval;
			timer_add(timer);
		}
	}

	if (timer)
		timer->time -= elapsed;
	timer_time = now;
}

static void alarm_handler(int signo, siginfo_t *si, void *uc)
{
	timer_triggered++;
}

void timer_init(void)
{
	struct sigaction sa;

	sa.sa_sigaction = alarm_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_SIGINFO;
	sigaction(SIGALRM, &sa, 0);

	timer_time = timer_now();

	timer_set(1);
}
