// Copyright 2008-2010  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "emu.h"
#include "platform.h"

#include "video.h"


u32 palette_rgb[256];
u32 screen[320*240];

int hide_page_1;
int hide_page_2;
int hide_sprites;
int show_fps = 1;
int trace_unknown_video = 1;


static const u8 sizes[] = { 8, 16, 32, 64 };
static const u8 colour_sizes[] = { 2, 4, 6, 8 };

static const u16 known_reg_bits[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,		// 2800..280f
	0xffff, 0xffff, 0x3fff, 0x011e, 0x3fff, 0x3fff,		// 2810..2815	text page 1
	0xffff, 0xffff, 0x3fff, 0x011e, 0x3fff, 0x3fff,		// 2816..281b	text page 2
	0x00ff,							// 281c		vcmp value
	0, 0, 0,						// 281d..281f
	0xffff, 0xffff, 0xffff,					// 2820..2822
	0, 0, 0, 0, 0, 0, 0,					// 2823..2829
	0x0003,							// 282a		blend level
	0, 0, 0, 0, 0,						// 282b..282f
	0x00ff,							// 2830		fade offset
	0, 0, 0, 0, 0,						// 2831..2835
	0xffff, 0xffff,						// 2836..2837   IRQ pos
	0, 0, 0, 0,						// 2838..283b
	0xffff,							// 283c		TV hue/saturation
	0, 0, 0,						// 283d..283f
	0, 0,							// 2840..2841
	0x0001,							// 2842
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,			// 2843..284f
	0, 0, 0, 0,						// 2850..2853
	0x0007,							// 2854		LCD control
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,			// 2855..285f
	0, 0,							// 2860..2861
	0x0007, 0x0007,						// 2862..2863
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,			// 2864..286f
	0x3fff, 0x03ff, 0x03ff,					// 2870..2872	DMA
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,			// 2873..287f
};

static const u16 known_sprite_bits[] = {
	0xffff, 0xffff, 0xffff, 0xffff	// actually only low 9 bits of X/Y, low 15 of attr
};

static void trace_unknown(u32 addr, u16 val, int is_read)
{
	if (addr >= 0x2800 && addr < 0x2800 + ARRAY_SIZE(known_reg_bits)) {
		if (val & ~known_reg_bits[addr - 0x2800])
			debug("*** UNKNOWN VIDEO REG BITS %s: %04x bits %04x\n",
			       is_read ? "READ" : "WRITE", addr,
			       val & ~known_reg_bits[addr - 0x2800]);
	} else if (addr >= 0x2900 && addr < 0x2a00) {
		if (val & ~0xffff)
			debug("*** UNKNOWN VIDEO HOFFSET BITS %s: %04x bits %04x\n",
			       is_read ? "READ" : "WRITE", addr, val & ~0x01ff);
	} else if (addr >= 0x2a00 && addr < 0x2b00) {
		if (val & ~0x00ff)
			debug("*** UNKNOWN VIDEO HCMP BITS %s: %04x bits %04x\n",
			       is_read ? "READ" : "WRITE", addr, val & ~0x00ff);
	} else if (addr >= 0x2b00 && addr < 0x2c00) {
		if (val & ~0xffff)
			debug("*** UNKNOWN VIDEO PALETTE BITS %s: %04x bits %04x\n",
			       is_read ? "READ" : "WRITE", addr, val & ~0xffff);
	} else if (addr >= 0x2c00 && addr < 0x3000) {
		if (val != 0xffff && val & ~known_sprite_bits[addr & 3])
			debug("*** UNKNOWN VIDEO SPRITE BITS %s: %04x bits %04x\n",
			       is_read ? "READ" : "WRITE", addr,
			       val & ~known_sprite_bits[addr & 3]);
	} else
		debug("*** UNKNOWN VIDEO %s: [%04x] = %04x\n",
		       is_read ? "READ" : "WRITE", addr, val);
}


static void do_dma(u32 len)
{
	u32 src = mem[0x2870];
	u32 dst = mem[0x2871] + 0x2c00;
	u32 j;

	for (j = 0; j < len; j++)
		mem[dst+j] = mem[src+j];

	mem[0x2872] = 0;
	mem[0x2863] |= 4;	// trigger video DMA IRQ
}

void video_store(u16 val, u32 addr)
{
	if (trace_unknown_video)
		trace_unknown(addr, val, 0);

	if (addr < 0x2900) {		// video regs
		switch (addr) {
		case 0x2810: case 0x2816:	// page 1,2 X scroll
			val &= 0x01ff;
			break;

		case 0x2811: case 0x2817:	// page 1,2 Y scroll
			val &= 0x00ff;
			break;

		case 0x2812 ... 0x2815:	// page 1 regs
		case 0x2818 ... 0x281b:	// page 2 regs
			break;

		case 0x281c:		// vcmp value
			break;

		case 0x2820 ... 0x2822:	// bitmap offsets
			break;

		case 0x282a:		// blend level
			break;

		case 0x2830:		// fade offset
			if ((mem[addr] & 0x00ff) != (val & 0x00ff))
				debug("*** TV FADE set to %02x\n", val & 0x00ff);
			break;

		case 0x2836:		// IRQ pos V
		case 0x2837:		// IRQ pos H
			val &= 0x01ff;
			break;

		case 0x283c:		// TV control 1 (hue/saturation)
			if ((mem[addr] & 0xff00) != (val & 0xff00))
				debug("*** TV HUE set to %02x\n", val >> 8);
			if ((mem[addr] & 0x00ff) != (val & 0x00ff))
				debug("*** TV SATURATION set to %02x\n", val & 0x00ff);
			break;

		case 0x283d:		// XXX TV control 2
			break;

		case 0x283e:		// XXX lightpen pos V
		case 0x283f:		// XXX lightpen pos H
			break;

		case 0x2842:		// XXX sprite enable
			break;

		case 0x2854: {		// LCD control
			static const char *lcd_colour_mode[4] = {
				"B/W", "16 grey levels", "4096 colours", "BAD"
			};
			if ((mem[addr] & 0x0003) != (val & 0x0003))
				debug("*** LCD COLOUR MODE set to %s\n",
				       lcd_colour_mode[val & 0x0003]);
			if ((mem[addr] & 0x0004) != (val & 0x0004))
				debug("*** LCD RESOLUTION set to %s\n",
				       (val & 0x0004) ? "320x240" : "160x100");
			break;
		}

		case 0x2862:		// video IRQ control
			break;

		case 0x2863:		// video IRQ status
			mem[addr] &= ~val;
			return;

		case 0x2870:		// video DMA src
		case 0x2871:		// video DMA dst
			break;

		case 0x2872:		// video DMA len
			do_dma(val);
			return;

		default:
			debug("VIDEO STORE %04x to %04x\n", val, addr);
		}
	} else if (addr < 0x2a00) {	// scroll per raster line
	} else if (addr < 0x2b00) {	// horizontal stretch
	} else if (addr < 0x2c00) {	// palette
	} else {			// sprites
	}

	mem[addr] = val;
}

u16 video_load(u32 addr)
{
	u16 val = mem[addr];

	if (trace_unknown_video)
		trace_unknown(addr, val, 1);

	if (addr < 0x2900) {
		switch (addr) {
		case 0x2810 ... 0x2815:	// page 1 regs
		case 0x2816 ... 0x281b:	// page 2 regs
			break;

		case 0x282a:		// blend level
			break;

		case 0x2838:		// current line
			return get_video_line();

		case 0x2842:		// XXX
			break;

		case 0x2862:		// video IRQ enable
			break;

		case 0x2863:		// video IRQ status
			break;

		case 0x2872:		// DMA len
			break;

		default:
			debug("VIDEO LOAD %04x from %04x\n", val, addr);
		}
	}

	return val;
}



void render_bitmap(u8 *dest, u32 pitch, u32 addr, u16 attr)
{
	u32 h = sizes[(attr & 0x00c0) >> 6];
	u32 w = sizes[(attr & 0x0030) >> 4];

	u32 yflipmask = attr & 0x0008 ? h - 1 : 0;
	u32 xflipmask = attr & 0x0004 ? w - 1 : 0;

	u32 nc = colour_sizes[attr & 0x0003];

	u32 m = addr;
	u32 bits = 0;
	u32 nbits = 0;

	for (u32 y = 0; y < h; y++) {
		u32 yy = y ^ yflipmask;

		for (u32 x = 0; x < w; x++) {
			u32 xx = x ^ xflipmask;

			bits <<= nc;
			if (nbits < nc) {
				u16 b = mem[m++ & 0x3fffff];
				b = (b << 8) | (b >> 8);
				bits |= b << (nc - nbits);
				nbits += 16;
			}
			nbits -= nc;

			dest[pitch*yy + xx] = bits >> 16;

			bits &= 0xffff;
		}
	}
}


void video_init(void)
{
	mem[0x2836] = 0xffff;
	mem[0x2837] = 0xffff;
}
