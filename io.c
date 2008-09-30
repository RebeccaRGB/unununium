// Copyright 2008  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include <stdio.h>

#include "types.h"
#include "emu.h"
#include "io.h"


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
		printf("IO STORE %04x to %04x\n", val, addr);
		break;

	case 0x3d59:		// IIC status/ack
		printf("IO STORE %04x to %04x\n", val, addr);
		break;

	case 0x3d5a:		// IIC access mode?  always 80, "combined mode"
		printf("IO STORE %04x to %04x\n", val, addr);
		break;

	case 0x3d5b:		// IIC device address
		printf("IO STORE %04x to %04x\n", val, addr);
		break;

	case 0x3d5c:		// IIC subadr
		printf("IO STORE %04x to %04x\n", val, addr);
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
