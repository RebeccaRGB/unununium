// Copyright 2008  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include <stdio.h>
#include <stdlib.h>

#include "types.h"
#include "emu.h"
#include "platform.h"
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

void io_store(u16 val, u32 addr)
{
	switch (addr) {
	case 0x3d00:		// GPIO special function select
		printf("IO STORE %04x to %04x\n", val, addr);
		break;

	case 0x3d07:		// port B data write
		printf("STORE %04x to %04x (port B)\n", val, addr);
		u32 bank = ((val & 0x80) >> 7) | ((val & 0x20) >> 4);
		switch_bank(bank);
		printf("switched to bank %x\n", bank);
		break;

	case 0x3d01 ... 0x3d06:
	case 0x3d08 ... 0x3d0f:	// ports A..C
		break;

	case 0x3d22:		// IRQ ack
		mem[addr] &= ~val;
		return;

	case 0x3d24:		// XXX
		if (val != 0x55aa)
			printf("IO STORE %04x to %04x\n", val, addr);
		break;

	case 0x3d2f:		// set DS
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
	case 0x3d01 ... 0x3d0f:		// GPIO
		break;

	case 0x3d22:			// IRQ status
		return mem[0x3d21];

	case 0x3d2c: case 0x3d2d:	// timers?
		return random();

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
