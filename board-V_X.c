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
// IOC8   controller pin 8    controller select, high active
// IOC9   controller pin 5     " "
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

// version:
// 0000  no vsmile
// 0001  no vsmile
// 0010  no text on vsmile
// 0011  no text on vsmile
// 0100  english
// 0101  english
// 0110  english
// 0111  chinese
// 1000  mexican
// 1001  english
// 1010  italian
// 1011  german
// 1100  spanish
// 1101  french
// 1110  english
// 1111  english


#include <stdio.h>

#include "types.h"
#include "emu.h"
#include "platform.h"

#include "board.h"


static int trace_gpio = 0;

static void init(void)
{
	if (mem[0x5675c] == 0x9311 && mem[0x5675e] == 0x4240 &&	// Winnie
	    mem[0x5675f] == 0x4e44)
		board->idle_pc = 0x5675c;

//	else
//		board->idle_pc = 0x4003a;	// studio, FIXME
}

static u32 bit(u16 x, u32 n)
{
	return (x >> n) & 1;
}

static u16 gpio(u32 n, u16 what, u16 push, u16 pull, u16 special)
{
//if (n == 2) what &= ~0x1400;
if (n == 2) what |= 0x1400;

if (n == 2) what = (what & ~0x000f) | 0x000d;	// french
//if (n == 2) what = (what & ~0x0010) | 0x0000;	// no vtech logo

	if (trace_gpio) {
		static u16 old[3][4];

		u32 i;
		for (i = 0; i < (n == 1 ? 8 : 16); i++)
			if (bit(what, i) != bit(old[n][0], i)
			   || bit(push, i) != bit(old[n][1], i)
			   || bit(pull, i) != bit(old[n][2], i)
			   || bit(special, i) != bit(old[n][3], i))
				debug("IO%c%d data=%x push=%x pull=%x special=%x\n",
				       'A' + n, i, bit(what, i), bit(push, i), bit(pull, i), bit(special, i));

		old[n][0] = what;
		old[n][1] = push;
		old[n][2] = pull;
		old[n][3] = special;
	}

//	debug("IO%c data=%04x push=%04x pull=%04x special=%04x\n",
//	       'A' + n, what, push, pull, special);

	return what;
}

static void uart_send(u8 x)
{
printf("--- uart_send(%02x)\n", x);
}

static u32 controller_in_count;
static u8 controller_input[8];

static u8 uart_recv(void)
{
	if (controller_in_count == 0) {
		if (button_up)
			controller_input[controller_in_count++] = 0x83;
		else if (button_down)
			controller_input[controller_in_count++] = 0x8b;
		else
			controller_input[controller_in_count++] = 0x80;


		if (button_left)
			controller_input[controller_in_count++] = 0xcb;
		else if (button_right)
			controller_input[controller_in_count++] = 0xc3;
		else
			controller_input[controller_in_count++] = 0xc0;

		u8 buttons = 0;
		if (button_A)
			buttons |= 0x08;
		if (button_B)
			buttons |= 0x04;
		if (button_C)
			buttons |= 0x02;

		controller_input[controller_in_count++] = 0x90 | buttons;
	}

	u8 x = controller_input[0];
	controller_in_count--;
	memcpy(controller_input, controller_input + 1, controller_in_count);

printf("V.x UART: %02x\n", x);
	return x;
}

struct board board_V_X = {
	.use_centered_coors = 1,

	.init = init,
	.gpio = gpio,
	.uart_send = uart_send,
	.uart_recv = uart_recv
};
