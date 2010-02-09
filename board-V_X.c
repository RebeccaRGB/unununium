// Copyright 2010  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt


// IOA not connected

// IOB0   CS#1 = a3; b3 = ROMCS#; o3 = cart CS1#
// IOB1   CS#2 = cart CS2#
// IOB2   CS#3 = a0; b0 = cart det (high if cart, pull to ROMCS#); o0 = sys CS#
// IOB3   reset switch
// IOB4   cart front 24?
// IOB5   power off
// IOB6   power off switch
// IOB7   power on switch

// IOC0   JP13 \ version jumper
// IOC1   JP12 |
// IOC2   JP11 |
// IOC3   JP10 /
// IOC4   JP14   logo
// IOC5   test point
// IOC6   amp power
// IOC7   system reset
// IOC8   controller pin 8
// IOC9   controller pin 5
// IOC10  controller pin 9 (also extint1)
// IOC11  N/C?
// IOC12  controller pin 6 (also extint2)
// IOC13  controller pins 4 and 7 ANDed together, pullups
// IOC14  controller pin 2, through a buffer
// IOC15  power / charger / whatever enable (top left)

// controller:
// pin1  ground
// pin2  system->controller
// pin3  power
// pin4  controller->system
// pin5  *
// pin6  *
// pin7  controller->system
// pin8  *
// pin9  *

// controller messages:
// 90+X: colour keys: 8=red 4=yellow 2=blue 1=green 0=release
// a0+X: other keys: 1=ok 2=quit 3=help 4=abc 0=release (1 for pen as well)
// 80+X: up: 3..7, 0=release stick (keypad uses 3)
// 88+X: down: 3..7
// c8+X: left: 3..7
// c0+X: right: 3..7


#include <stdio.h>

#include "types.h"
#include "emu.h"
#include "platform.h"

#include "board.h"


static void init(void)
{
	if (mem[0x5675c] == 0x9311 && mem[0x5675e] == 0x4240 &&	// Winnie
	    mem[0x5675f] == 0x4e44)
		board->idle_pc = 0x5675c;

	else
		board->idle_pc = 0x4003a;	// studio, FIXME
}

static u16 gpio(u32 n, u16 what, u16 push, u16 pull, u16 special)
{
	return what;
}

struct board board_V_X = {
	.use_centered_coors = 1,

	.init = init,
	.gpio = gpio
};
