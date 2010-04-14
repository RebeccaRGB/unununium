// Copyright 2008-2010  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include "types.h"
#include "platform.h"
#include "i2c.h"
	#include <stdio.h>

#include "board.h"


static struct i2c_bus *i2c_bus;

static u16 gpio(u32 n, u16 what, __unused u16 push, __unused u16 pull, __unused u16 special)
{
//	debug("--->  PORT %c  what=%04x push=%04x pull=%04x\n",
//	        'A' + n, what, push, pull);

	if (n == 0) {
		if (button_up)
			what |= 0x8000;
		if (button_down)
			what |= 0x4000;
		if (button_left)
			what |= 0x2000;
		if (button_right)
			what |= 0x1000;
		if (button_A)
			what |= 0x0800;
		if (button_B)
			what |= 0x0400;
		if (button_menu)
			what |= 0x0020;
	}

	if (n == 2) {
		int sda = what & 1;
		int scl = (what >> 1) & 1;
//debug("SDA=%d SCL=%d\n", sda, scl);
		sda = i2c_bitbang(i2c_bus, sda, scl);
		what = (what & ~1) | sda;
	}

	return what;
}

static void init(void)
{
	i2c_bus = i2c_bitbang_bus_create();
	i2c_eeprom_create(i2c_bus, 0x200, 0xa0, "WAL");
}

struct board board_WAL = {
	.idle_pc = 0xb1c6,

	.init = init,
	.gpio = gpio
};
