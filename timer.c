// Copyright 2010  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include <sys/time.h>
#include <signal.h>
#include <unistd.h>

#include "types.h"
#include "platform.h"

#include "timer.h"


volatile u32 timer_triggered;

void timer_set(void)
{
	struct itimerval it;

	it.it_value.tv_sec = 0;
	it.it_value.tv_usec = 1000000/250;
	it.it_interval.tv_sec = 0;
	it.it_interval.tv_usec = 0;

	int err = setitimer(ITIMER_REAL, &it, 0);
	if (err)
		fatal("setitimer failed");
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

	timer_set();
}
