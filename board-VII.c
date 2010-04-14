// Copyright 2008-2010  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include <stdlib.h>
#include <stdio.h>

#include "types.h"
#include "emu.h"
#include "platform.h"

#include "board.h"


static u32 current_bank = -1;
static int controller_should_be_rotated;

static void switch_bank(u32 bank)
{
	if (bank == current_bank)
		return;
	current_bank = bank;

	render_kill_cache();

	read_rom(N_MEM*bank);
//	memset(ever_ran_this, 0, N_MEM);

	board->idle_pc = 0;
	if (mem[0x19792] == 0x4311 && mem[0x19794] == 0x4e43)	// VII bank 0
		board->idle_pc = 0x19792;
	if (mem[0x21653] == 0x4311 && mem[0x21655] == 0x4e43)	// VII bank 2
		board->idle_pc = 0x21653;
	if (mem[0x42daa] == 0x4311 && mem[0x42dac] == 0x4e43) {	// VC1
		board->idle_pc = 0x42daa;
		controller_should_be_rotated = 1;
		//mem[0x38ecf] = 0;		// no life lost in LP
		//mem[0x38eb1] = mem[0xb8eb0];	// no upgrades lost in LP
	}
	if (mem[0x3ecb9] == 0x4311 && mem[0x3ecbb] == 0x4e43) {	// VC2
		board->idle_pc = 0x3ecb9;
		controller_should_be_rotated = 1;
	}
}

static void init(void)
{
	switch_bank(0);
}

static u16 gpio(u32 n, u16 what, __unused u16 push, __unused u16 pull, __unused u16 special)
{
	if (n == 1) {
		debug("STORE %04x to port B\n", what);
		u32 bank = ((what & 0x80) >> 7) | ((what & 0x20) >> 4);
		switch_bank(bank);
		debug("switched to bank %x\n", bank);
	}

	return what;
}

static void uart_send(__unused u8 x)
{
//debug("--- uart_send(%02x)\n", x);
}

static u32 controller_in_count;
static u8 controller_input[8];

static u8 uart_recv(void)
{
	if (controller_in_count == 0) {
		u8 buttons = 0;
		if (controller_should_be_rotated) {
			if (button_up)
				buttons |= 0x08;
			if (button_down)
				buttons |= 0x04;
			if (button_left)
				buttons |= 0x01;
			if (button_right)
				buttons |= 0x02;
		} else {
			if (button_up)
				buttons |= 0x01;
			if (button_down)
				buttons |= 0x02;
			if (button_left)
				buttons |= 0x04;
			if (button_right)
				buttons |= 0x08;
		}
		if (button_A)
			buttons |= 0x10;
		if (button_B)
			buttons |= 0x20;

		controller_input[0] = buttons;

		u32 x = random() & 0x3ff;
		u32 y = random() & 0x3ff;
		u32 z = random() & 0x3ff;

		controller_input[1] = x;
		controller_input[2] = y;
		controller_input[3] = z;

		x >>= 8;
		y >>= 8;
		z >>= 8;

		controller_input[4] = 0;
		controller_input[5] = (z << 4) | (y << 2) | x;

		controller_input[6] = 0xff;
		controller_input[7] = 0;

		controller_in_count = 8;
	}

	u8 x = controller_input[8 - controller_in_count];
	controller_in_count--;

	return x;
}

struct board board_VII = {
	.use_centered_coors = 1,

	.init = init,
	.gpio = gpio,
	.uart_send = uart_send,
	.uart_recv = uart_recv
};
