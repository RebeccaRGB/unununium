// Copyright 2008-2010  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include <stdio.h>

#include "types.h"
#include "emu.h"
#include "platform.h"

#include "board.h"


static u32 current_bank = -1;

static void switch_bank(u32 bank)
{
	if (bank == current_bank)
		return;
	current_bank = bank;

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
		//mem[0x38ecf] = 0;	// no life lost in LP
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

static u16 gpio(u32 n, u16 what, u16 push, u16 pull, u16 special)
{
	if (n == 1) {
		printf("STORE %04x to port B\n", what);
		u32 bank = ((what & 0x80) >> 7) | ((what & 0x20) >> 4);
		switch_bank(bank);
		fprintf(stderr, "switched to bank %x\n", bank);
	}

	return what;
}

struct board board_VII = {
	.use_centered_coors = 1,

	.init = init,
	.gpio = gpio
};
