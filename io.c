// Copyright 2008-2010  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include <stdio.h>
#include <stdlib.h>

#include "types.h"
#include "emu.h"
#include "platform.h"
#include "board.h"
#include "timer.h"

#include "io.h"

int trace_unknown_io = 1;

static const u16 known_reg_bits[] = {
	0x001f,							// 3d00		GPIO control
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff,			// 3d01..3d05	IOA
	0x00ff, 0x00ff, 0x00ff, 0x00ff, 0x00ff,			// 3d06..3d0a	IOB
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff,			// 3d0b..3d0f	IOC
	0x000f,							// 3d10		timebase freq
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,		// 3d11..3d1f
	0x4006,							// 3d20		system control
	0x3ffb,							// 3d21		IRQ control
	0x7fff,							// 3d22		IRQ status
	0x003e,							// 3d23		memory control
	0xffff,							// 3d24		watchdog
	0x2002,							// 3d25		ADC control
	0, 0,							// 3d26..3d27
	0xffff,							// 3d28		sleep
	0x0080,							// 3d29		wakeup source
	0x00ff,							// 3d2a		wakeup delay
	0x0001,							// 3d2b		PAL/NTSC
	0, 0,							// 3d2c..3d2d
	0x0007,							// 3d2e		FIQ control
	0x003f,							// 3d2f		DS
	0x00ef,							// 3d30		UART control
	0x0003,							// 3d31		UART status
	0,							// 3d32
	0x00ff, 0x00ff,						// 3d33..3d34	UART baud rate
	0x00ff,							// 3d35		UART TX
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,				// 3d36..3d3f
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,		// 3d40..3d4f
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,		// 3d50..3d5f
};

static const u16 known_dma_reg_bits[] = {
	0xffff,		// source low
	0x003f,		// source high
	0x3fff,		// len
	0x3fff,		// dest
};

static void trace_unknown(u32 addr, u16 val, int is_read)
{
	if (addr >= 0x3d00 && addr < 0x3d00 + ARRAY_SIZE(known_reg_bits)) {
		if (val & ~known_reg_bits[addr - 0x3d00])
			debug("*** UNKNOWN IO REG BITS %s: %04x bits %04x\n",
			       is_read ? "READ" : "WRITE", addr,
			       val & ~known_reg_bits[addr - 0x3d00]);
	} else if (addr >= 0x3e00 && addr < 0x3e00 + ARRAY_SIZE(known_dma_reg_bits)) {
		if (val & ~known_dma_reg_bits[addr - 0x3e00])
			debug("*** UNKNOWN DMA REG BITS %s: %04x bits %04x\n",
			       is_read ? "READ" : "WRITE", addr,
			       val & ~known_reg_bits[addr - 0x3e00]);
	} else
		debug("*** UNKNOWN IO %s: [%04x] = %04x\n",
		       is_read ? "READ" : "WRITE", addr, val);
}


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
	u16 buffer  = mem[0x3d02 + 5*n];
	u16 dir     = mem[0x3d03 + 5*n];
	u16 attr    = mem[0x3d04 + 5*n];
	u16 special = mem[0x3d05 + 5*n];

	u16 push = dir;
	u16 pull = (~dir) & (~attr);
	u16 what = (buffer & (push | pull));
	what ^= (dir & ~attr);
	what &= ~special;

	if (board->gpio)
		what = board->gpio(n, what, push, pull, special);

	mem[0x3d01 + 5*n] = what;
}

static void do_dma(u32 len)
{
	u32 src = ((mem[0x3e01] & 0x3f) << 16) | mem[0x3e00];
	u32 dst = mem[0x3e03] & 0x3fff;
	u32 j;

	for (j = 0; j < len; j++)
		mem[dst+j] = mem[src+j];

	mem[0x3e02] = 0;
}

static void do_tmb1(void)
{
	mem[0x3d22] |= 0x0001;
}

static void do_tmb2(void)
{
	mem[0x3d22] |= 0x0002;
}

static struct timer timer_tmb1 = {
	.name = "TMB1",
	.interval = 27000000/8,
	.run = do_tmb1
};

static struct timer timer_tmb2 = {
	.name = "TMB2",
	.interval = 27000000/128,
	.run = do_tmb2
};

void io_store(u16 val, u32 addr)
{
	if (trace_unknown_io)
		trace_unknown(addr, val, 0);

	switch (addr) {
	case 0x3d00:		// GPIO special function select
		if ((mem[addr] & 0x0001) != (val & 0x0001))
			debug("*** IOA SPECIAL set #%d\n", val & 0x0001);
		if ((mem[addr] & 0x0002) != (val & 0x0002))
			debug("*** IOB SPECIAL set #%d\n", (val & 0x0002) >> 1);
		if ((mem[addr] & 0x0004) != (val & 0x0004))
			debug("*** IOA WAKE %sabled\n",
			       (val & 0x0004) ? "en" : "dis");
		if ((mem[addr] & 0x0008) != (val & 0x0008))
			debug("*** IOB WAKE %sabled\n",
			       (val & 0x0008) ? "en" : "dis");
		if ((mem[addr] & 0x0010) != (val & 0x0010))
			debug("*** IOC WAKE %sabled\n",
			       (val & 0x0010) ? "en" : "dis");
		break;

// 3d01 3d06 3d0b	data
// 3d02 3d07 3d0c	buffer
// 3d03 3d08 3d0d	dir
// 3d04 3d09 3d0e	attrib
// 3d05 3d0a 3d0f	special

	case 0x3d01:
	case 0x3d06:
	case 0x3d0b:		// port A, B, C data
		addr++;
		// fallthrough: write to buffer instead

	case 0x3d05: case 0x3d0a: case 0x3d0f:	// GPIO special function select
		// fallthrough

	case 0x3d02 ... 0x3d04:	// port A
	case 0x3d07 ... 0x3d09:	// port B
	case 0x3d0c ... 0x3d0e:	// port C
		mem[addr] = val;
		do_gpio(addr);
		return;

	case 0x3d10:		// timebase control
		if ((mem[addr] & 0x0003) != (val & 0x0003)) {
			u16 hz = 8 << (val & 0x0003);
			debug("*** TMB1 FREQ set to %dHz\n", hz);
			timer_tmb1.interval = 27000000 / hz;
		}
		if ((mem[addr] & 0x000c) != (val & 0x000c)) {
			u16 hz = 128 << ((val & 0x000c) >> 2);
			debug("*** TMB2 FREQ set to %dHz\n", hz);
			timer_tmb2.interval = 27000000 / hz;
		}
		break;

	// case 0x3d11: timebase clear
	// case 0x3d12:
	// case 0x3d13:
	// case 0x3d14: timer A enable
	// case 0x3d15: timer A IRQ ack
	// case 0x3d16:
	// case 0x3d17:
	// case 0x3d18: timer B enable
	// case 0x3d19: timer B IRQ ack
	// case 0x3d1a:
	// case 0x3d1b:
	// case 0x3d1c:
	// case 0x3d1d:
	// case 0x3d1e:
	// case 0x3d1f:

	case 0x3d20:		// system control
		if ((mem[addr] & 0x4000) != (val & 0x4000))
			debug("*** SLEEP MODE %sabled\n",
			       (val & 0x4000) ? "en" : "dis");
		break;

	case 0x3d21:		// IRQ control
		break;

	case 0x3d22:		// IRQ ack
		mem[addr] &= ~val;
		return;

	case 0x3d23:		// memory control
		if ((mem[addr] & 0x0006) != (val & 0x0006))
			debug("*** MEMORY WAIT STATE switched to %d\n", (val & 0x0006) >> 1);
		if ((mem[addr] & 0x0038) != (val & 0x0038))
			debug("*** BUS ARBITER switched to mode %d\n", (val & 0x0038) >> 3);
		break;

	case 0x3d24:		// watchdog
		if (val != 0x55aa)
			debug("*** WEIRD WATCHDOG WRITE: %04x\n", val);
		break;

	case 0x3d25:		// ADC control
		if ((mem[addr] & 0x0002) != (val & 0x0002))
			debug("*** ADC %sabled\n",
			       (val & 0002) ? "dis" : "en");
		mem[addr] = ((val | mem[addr]) & 0x2000) ^ val;
		return;

	// case 0x3d26:
	// case 0x3d27: ADC data

	case 0x3d28:		// sleep
		if (val != 0xaa55)
			debug("*** WEIRD SLEEP WRITE: %04x\n", val);
		break;

	case 0x3d29:		// wakeup source
		if ((mem[addr] & 0x0080) != (val & 0x0080))
			debug("*** WAKE ON KEY %sabled\n",
			       (val & 0x0080) ? "en" : "dis");
		break;

	case 0x3d2a:		// wakeup delay
		if ((mem[addr] & 0x00ff) != (val & 0x00ff))
			debug("*** WAKE DELAY %d\n", val & 0x00ff);
		break;

	// case 0x3d2b: PAL/NTSC
	// case 0x3d2c: PRNG #1
	// case 0x3d2d: PRNG #2

	case 0x3d2e:		// FIQ select
		if ((mem[addr] & 0x0007) != (val & 0x0007))
			debug("*** FIQ select %d\n", val & 0x0007);
		break;

	case 0x3d2f:		// DS
		set_ds(val);
		break;

	case 0x3d30: {		// UART control
		static const char *mode[4] = {
			"0", "1", "odd", "even"
		};
		if ((mem[addr] & 0x0001) != (val & 0x0001))
			debug("*** UART RX IRQ %sabled\n",
			       (val & 0x0001) ? "en" : "dis");
		if ((mem[addr] & 0x0002) != (val & 0x0002))
			debug("*** UART TX IRQ %sabled\n",
			       (val & 0x0002) ? "en" : "dis");
		if ((mem[addr] & 0x000c) != (val & 0x000c))
			debug("*** UART 9th bit is %s\n",
			       mode[(val & 0x000c) >> 2]);
		if ((mem[addr] & 0x0020) != (val & 0x0020))
			debug("*** UART 9th bit %sabled\n",
			       (val & 0x0020) ? "en" : "dis");
		if ((mem[addr] & 0x0040) != (val & 0x0040))
			debug("*** UART RX %sabled\n",
			       (val & 0x0040) ? "en" : "dis");
		if ((mem[addr] & 0x0080) != (val & 0x0080))
			debug("*** UART TX %sabled\n",
			       (val & 0x0080) ? "en" : "dis");
		break;
	}

	case 0x3d31:		// UART status
		mem[addr] ^= (mem[addr] & 0x0003 & val);
		return;

	// case 0x3d32:		// UART reset

	case 0x3d33:		// UART baud rate low byte
		debug("*** UART BAUD RATE is %u\n", 27000000 / 16 / (0x10000 - (mem[0x3d34] << 8) - val));
		break;

	case 0x3d34:		// UART baud rate high byte
		debug("*** UART BAUD RATE is %u\n", 27000000 / 16 / (0x10000 - (val << 8) - mem[0x3d33]));
		break;

	case 0x3d35:		// UART TX data
		if (board->uart_send)
			board->uart_send(val);
		else
			debug("UART write %02x (not hooked up)\n", val);
		return;


	// case 0x3d36: UART RX data
	// case 0x3d37:
	// case 0x3d38:
	// case 0x3d39:
	// case 0x3d3a:
	// case 0x3d3b:
	// case 0x3d3c:
	// case 0x3d3d:
	// case 0x3d3e:
	// case 0x3d3f:
	// case 0x3d40: SPI control
	// case 0x3d41: SPI data
	// case 0x3d42: SPI clock
	// case 0x3d43:
	// case 0x3d44:
	// case 0x3d45:
	// case 0x3d46:
	// case 0x3d47:
	// case 0x3d48:
	// case 0x3d49:
	// case 0x3d4a:
	// case 0x3d4b:
	// case 0x3d4c:
	// case 0x3d4d:
	// case 0x3d4e:
	// case 0x3d4f:
	// case 0x3d50:
	// case 0x3d51:
	// case 0x3d52:
	// case 0x3d53:
	// case 0x3d54:
	// case 0x3d55:
	// case 0x3d56:
	// case 0x3d57:

	case 0x3d58:		// IIC command
		mem[addr] = val;
		do_i2c();
		return;

	case 0x3d59:		// IIC status/ack
		mem[addr] &= ~val;
		return;

	case 0x3d5a:		// IIC access mode?  always 80, "combined mode"
		break;

	case 0x3d5b:		// IIC device address
	case 0x3d5c:		// IIC subadr
		break;

	case 0x3d5d:		// IIC data out
		break;

	case 0x3d5e:		// IIC data in
		break;

	case 0x3d5f:		// IIC controller mode
		break;

	case 0x3e00:		// DMA source addr low
	case 0x3e01:		// DMA source addr high
		break;

	case 0x3e02:		// DMA size
		do_dma(val);
		return;

	case 0x3e03:		// DMA target addr
		break;

	default:
		;
	}

	mem[addr] = val;
}

u16 io_load(u32 addr)
{
	u16 val = mem[addr];

	if (trace_unknown_io)
		trace_unknown(addr, val, 1);

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

	case 0x3d1c:			// video line counter
		val = get_video_line();
		break;

	case 0x3d21:			// IRQ control
		break;

	case 0x3d22:			// IRQ status
		break;

	case 0x3d2b:			// PAL/NTSC
		break;

	case 0x3d2c: case 0x3d2d:	// PRNG
		return random();

	case 0x3d2f:			// DS
		return get_ds();

	case 0x3d31:			// UART status     FIXME
		return 3;

	case 0x3d36:			// UART RX data
		if (board->uart_recv)
			val = board->uart_recv();
		else {
			debug("UART READ (not hooked up)\n");
			val = 0;
		}
		return val;

	case 0x3d59:			// IIC status
		break;

	case 0x3d5e:			// IIC data in
		break;

	default:
		;
	}

	return val;
}

void io_init(void)
{
	timer_add(&timer_tmb1);
	timer_add(&timer_tmb2);
}
