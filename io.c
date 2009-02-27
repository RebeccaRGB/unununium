// Copyright 2008,2009  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include <stdio.h>
#include <stdlib.h>

#include "types.h"
#include "emu.h"
#include "platform.h"
#include "board.h"

#include "io.h"


static int uart_in_count = 0;


static u8 seeprom[0x400];

static void do_i2c(void)
{
	u32 addr;

	addr = (mem[0x3d5b] & 0x06) << 7;
	addr |= mem[0x3d5c] & 0xff;

	if (mem[0x3d58] & 0x40)
		mem[0x3d5e] = seeprom[addr];
	else
		seeprom[addr] = mem[0x3d5d];

	mem[0x3d59] |= 1;
}

static void do_gpio(u32 addr)
{
	u32 n = (addr - 0x3d01) / 5;
	u16 buffer = mem[0x3d02 + 5*n];
	u16 dir    = mem[0x3d03 + 5*n];
	u16 attr   = mem[0x3d04 + 5*n];

	u16 push = dir;
	u16 pull = (~dir) & (~attr);
	u16 what = (buffer & (push | pull)) ^ (dir & ~attr);

	if (board->do_gpio)
		what = board->do_gpio(n, what, push, pull);

	mem[0x3d01 + 5*n] = what;
}

void io_store(u16 val, u32 addr)
{
	switch (addr) {
	case 0x3d00:		// GPIO special function select
		printf("IO STORE %04x to %04x\n", val, addr);
		break;

// 3d01 3d06 3d0b	data
// 3d02 3d07 3d0c	buffer
// 3d03 3d08 3d0d	dir
// 3d04 3d09 3d0e	attrib
// 3d05 3d0a 3d0f	irq? latch?

	case 0x3d01:
	case 0x3d06:
	case 0x3d0b:		// port A, B, C data
		addr++;
		// fallthrough: write to buffer instead

	case 0x3d02 ... 0x3d05:	// port A
	case 0x3d07 ... 0x3d0a:	// port B
	case 0x3d0c ... 0x3d0f:	// port C
//		if (addr == 0x3d07) {		// port B buffer
//			printf("STORE %04x to %04x (port B)\n", val, addr);
//			u32 bank = ((val & 0x80) >> 7) | ((val & 0x20) >> 4);
//			switch_bank(bank);
//			printf("switched to bank %x\n", bank);
//		}
		mem[addr] = val;
		do_gpio(addr);
		return;

	// case 0x3d10:
	// case 0x3d11:
	// case 0x3d14:
	// case 0x3d15:
	// case 0x3d18:
	// case 0x3d19:
	// case 0x3d21:

	case 0x3d22:		// IRQ ack
		mem[addr] &= ~val;
		return;

	// case 0x3d23:

	case 0x3d24:		// watchdog
		if (val != 0x55aa)
			printf("IO STORE %04x to %04x\n", val, addr);
		break;

	// case 0x3d2b:

	case 0x3d2f:		// DS
		set_ds(val);
		break;

	case 0x3d31:		// XXX UART
		if (val != 0x0003)
			printf("IO STORE %04x to %04x\n", val, addr);
		break;

	case 0x3d33:		// UART baud rate
		printf("SET UART BAUD RATE to %d\n", 27000000 / (0x10000 - val));
		break;

	case 0x3d35:		// UART TX data
		break;

	case 0x3d58:		// IIC command
		mem[addr] = val;
		do_i2c();
		return;

	case 0x3d59:		// IIC status/ack
		mem[addr] &= ~val;
		return;

	case 0x3d5a:		// IIC access mode?  always 80, "combined mode"
		printf("IO STORE %04x to %04x\n", val, addr);
		break;

	case 0x3d5b:		// IIC device address
	case 0x3d5c:		// IIC subadr
		break;

	case 0x3d5d:		// IIC data out
		printf("IO STORE %04x to %04x\n", val, addr);
		break;

	case 0x3d5e:		// IIC data in
		printf("IO STORE %04x to %04x\n", val, addr);
		break;

	case 0x3d5f:		// IIC controller mode
		printf("IO STORE %04x to %04x\n", val, addr);
		break;

	default:
		printf("IO STORE %04x to %04x\n", val, addr);
	}

	mem[addr] = val;
}

u16 io_load(u32 addr)
{
	u16 val = mem[addr];

	switch (addr) {
	case 0x3d01:
	case 0x3d06:
	case 0x3d0b:			// GPIO
		do_gpio(addr);
		return mem[addr];

	case 0x3d02 ... 0x3d05:
	case 0x3d07 ... 0x3d0a:
	case 0x3d0c ... 0x3d0f:		// GPIO
		break;

	case 0x3d22:			// IRQ status
		return mem[0x3d21];

	case 0x3d2c: case 0x3d2d:	// timers?
		return random();

	case 0x3d2f:			// DS
		return get_ds();

	case 0x3d31:			// UART (status?)
		return 3;

	case 0x3d36:			// UART data in
		val = controller_input[uart_in_count];
		uart_in_count = (uart_in_count + 1) % 8;
		return val;

	case 0x3d59:			// IIC status
		break;

	case 0x3d5e:			// IIC data in
		break;

	default:
		printf("IO LOAD %04x from %04x\n", val, addr);
	}

	return val;
}
