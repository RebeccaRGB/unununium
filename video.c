// Copyright 2008-2010  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "emu.h"
#include "platform.h"
#include "board.h"

#include "video.h"


u32 palette_rgb[256];
u32 screen[320*240];

int hide_page_1;
int hide_page_2;
int hide_sprites;
int show_fps = 0;
int trace_unknown_video = 1;


static void fill_rect(u32 x, u32 y, u32 w, u32 h, u32 rgb)
{
	u32 xx, yy;
	for (yy = y; yy < y + h; yy++)
		for (xx = x; xx < x + w; xx++)
			screen[320*yy + xx] = rgb;
}

static void do_show_fps(void)
{
	static u32 fps = 0;

	const int per_n = 10;
	static int count = 0;
	count++;
	if (count == per_n) {
		static u32 last = 0;
		u32 now = get_realtime();
		if (last)
			fps = 1000000 * per_n / (now - last);
		last = now;
		count = 0;
	}

	if (fps > 50)
		fps = 50;

	fill_rect(3, 3, 7, 52, 0xffffff);
	fill_rect(4, 4, 5, 50 - fps, 0x000000);
	fill_rect(4, 54 - fps, 5, fps, 0x00ff00);
}

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
			printf("*** UNKNOWN VIDEO REG BITS %s: %04x bits %04x\n",
			       is_read ? "READ" : "WRITE", addr,
			       val & ~known_reg_bits[addr - 0x2800]);
	} else if (addr >= 0x2900 && addr < 0x2a00) {
		if (val & ~0xffff)
			printf("*** UNKNOWN VIDEO HOFFSET BITS %s: %04x bits %04x\n",
			       is_read ? "READ" : "WRITE", addr, val & ~0x01ff);
	} else if (addr >= 0x2a00 && addr < 0x2b00) {
		if (val & ~0x00ff)
			printf("*** UNKNOWN VIDEO HCMP BITS %s: %04x bits %04x\n",
			       is_read ? "READ" : "WRITE", addr, val & ~0x00ff);
	} else if (addr >= 0x2b00 && addr < 0x2c00) {
		if (val & ~0xffff)
			printf("*** UNKNOWN VIDEO PALETTE BITS %s: %04x bits %04x\n",
			       is_read ? "READ" : "WRITE", addr, val & ~0xffff);
	} else if (addr >= 0x2c00 && addr < 0x3000) {
		if (val != 0xffff && val & ~known_sprite_bits[addr & 3])
			printf("*** UNKNOWN VIDEO SPRITE BITS %s: %04x bits %04x\n",
			       is_read ? "READ" : "WRITE", addr,
			       val & ~known_sprite_bits[addr & 3]);
	} else
		printf("*** UNKNOWN VIDEO %s: [%04x] = %04x\n",
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
			if (mem[addr] != val)
				bitmap_cache_clear();
			break;

		case 0x282a:		// blend level
			break;

		case 0x2830:		// fade offset
			if ((mem[addr] & 0x00ff) != (val & 0x00ff))
				printf("*** TV FADE set to %02x\n", val & 0x00ff);
			break;

		case 0x2836:		// IRQ pos V
		case 0x2837:		// IRQ pos H
			val &= 0x01ff;
			break;

		case 0x283c:		// TV control 1 (hue/saturation)
			if ((mem[addr] & 0xff00) != (val & 0xff00))
				printf("*** TV HUE set to %02x\n", val >> 8);
			if ((mem[addr] & 0x00ff) != (val & 0x00ff))
				printf("*** TV SATURATION set to %02x\n", val & 0x00ff);
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
				printf("*** LCD COLOUR MODE set to %s\n",
				       lcd_colour_mode[val & 0x0003]);
			if ((mem[addr] & 0x0004) != (val & 0x0004))
				printf("*** LCD RESOLUTION set to %s\n",
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
			printf("VIDEO STORE %04x to %04x\n", val, addr);
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
			printf("VIDEO LOAD %04x from %04x\n", val, addr);
		}
	}

	return val;
}


static u8 *bitmap_cache[65536];
static u16 bitmap_cache_attr[65536];

static u32 bitmap_cache_hash(u32 page, u16 tile, u16 attr)
{
	return (page << 14) ^ tile ^ (attr & 0x0fff);
}

void bitmap_cache_clear(void)
{
	u32 i;
	for (i = 0; i < 65536; i++)
		if (bitmap_cache[i]) {
			free(bitmap_cache[i]);
			bitmap_cache[i] = 0;
		}
}

static u8 *bitmap_cache_make(u32 page, u16 tile, u16 attr)
{
	u32 hash = bitmap_cache_hash(page, tile, attr);
	if (bitmap_cache[hash] && bitmap_cache_attr[hash] == (attr & 0x0fff))
		return bitmap_cache[hash];

	u32 h = sizes[(attr & 0x00c0) >> 6];
	u32 w = sizes[(attr & 0x0030) >> 4];

	u8 *p = malloc(h * w);
	free(bitmap_cache[hash]);
	bitmap_cache[hash] = p;
	bitmap_cache_attr[hash] = attr & 0x0fff;

	u32 yflipmask = attr & 0x0008 ? h - 1 : 0;
	u32 xflipmask = attr & 0x0004 ? w - 1 : 0;

	u32 nc = colour_sizes[attr & 0x0003];

	u32 palette_offset = (attr & 0x0f00) >> 4;
	palette_offset >>= nc;
	palette_offset <<= nc;

	u32 bitmap = 0x40*mem[0x2820 + page];
	u32 m = bitmap + nc*w*h/16*tile;
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

			u32 pal = palette_offset | (bits >> 16);
			bits &= 0xffff;

			p[w*yy + xx] = pal;
		}
	}

	return p;
}


static inline u8 mix_channel(u8 old, u8 new)
{
	u8 alpha = mem[0x282a];
	return ((4 - alpha)*old + alpha*new) / 4;
}

static inline void mix_pixel(u32 offset, u32 rgb)
{
	u32 x = 0;
	u32 i;
	for (i = 0; i < 3; i++) {
		u8 old = (screen[offset] & pixel_mask[i]) >> pixel_shift[i];
		u8 new = (rgb & pixel_mask[i]) >> pixel_shift[i];
		x |= (mix_channel(old, new) << pixel_shift[i]) & pixel_mask[i];
	}
	screen[offset] = x;
}

static void blit(u32 page, u32 xoff, u32 yoff, u32 attr, u32 ctrl, u16 tile)
{
	u8 *p = bitmap_cache_make(page, tile, attr);

	u32 h = sizes[(attr & 0x00c0) >> 6];
	u32 w = sizes[(attr & 0x0030) >> 4];

	for (u32 y = 0; y < h; y++) {
		u32 yy = (yoff + y) & 0x1ff;

		for (u32 x = 0; x < w; x++) {
			u32 xx = (xoff + x) & 0x1ff;

			u32 pal = *p++;

			if ((ctrl & 0x0010) && yy < 240)
				xx = (xx - mem[0x2900 + yy]) & 0x01ff;

			//if ((ctrl & 0x0020) && yy < 240)	// hcmp
			//	xx = (xx - mem[0x2a00 + yy]) & 0x01ff;//WRONG

			if (xx < 320 && yy < 240) {
				u16 rgb = mem[0x2b00 + pal];
				if ((rgb & 0x8000) == 0) {
					if (attr & 0x4000 || ctrl & 0x0100)
						mix_pixel(xx + 320*yy, palette_rgb[pal]);
					else
						screen[xx + 320*yy] = palette_rgb[pal];
				}
			}
		}
	}
}

static void __noinline blit_page(u32 page, u32 depth)
{
	u16 *regs = &mem[0x2810 + 6*page];

	u32 xscroll = regs[0];
	u32 yscroll = regs[1];
	u32 attr = regs[2];
	u32 ctrl = regs[3];
	u32 tilemap = regs[4];
	u32 palette_map = regs[5];

	if ((ctrl & 8) == 0)
		return;

	if ((attr & 0x3000) >> 12 != depth)
		return;

	u32 h = sizes[(attr & 0x00c0) >> 6];
	u32 w = sizes[(attr & 0x0030) >> 4];

	u32 hn = 256/h;
	u32 wn = 512/h;

	u32 x0, y0;
	for (y0 = 0; y0 < hn; y0++)
		for (x0 = 0; x0 < wn; x0++) {
			u32 which = (ctrl & 4) ? 0 : x0 + wn*y0;
			u16 tile = mem[tilemap + which];

			if (tile == 0)
				continue;

			u16 palette = mem[palette_map + which/2];
			if (which & 1)
				palette >>= 8;

			u32 tileattr = attr;
			u32 tilectrl = ctrl;
			if ((ctrl & 2) == 0) {
// -(1) bld(1) flip(2) pal(4)
				tileattr &= ~0x000c;
				tileattr |= (palette >> 2) & 0x000c;	// flip

				tileattr &= ~0x0f00;
				tileattr |= (palette << 8) & 0x0f00;	// palette

				tilectrl &= ~0x0100;
				tilectrl |= (palette << 2) & 0x0100;	// blend
			}

			u32 yy = ((h*y0 - yscroll + 0x10) & 0xff) - 0x10;
			u32 xx = (w*x0 - xscroll) & 0x1ff;

			blit(page, xx, yy, tileattr, tilectrl, tile);
		}
}

static void blit_sprite(u32 depth, u16 *sprite)
{
	u16 tile, attr;
	s16 x, y;

	tile = sprite[0];
	x = sprite[1];
	y = sprite[2];
	attr = sprite[3];
//if (tile >= 0x8000)
//	printf("UH-OH: %04x %04x %04x %04x\n", tile, x, y, attr);

	if (tile == 0)
		return;

	if ((u32)(attr & 0x3000) >> 12 != depth)
		return;

	if (board->use_centered_coors) {
		x = 160 + x;
		y = 120 - y;

		u32 h = sizes[(attr & 0x00c0) >> 6];
		u32 w = sizes[(attr & 0x0030) >> 4];

		x -= (w/2);
		y -= (h/2) - 8;
	}

	x = x & 0x1ff;
	y = y & 0x1ff;

	blit(2, x, y, attr, 0, tile);
}

static void __noinline blit_sprites(u32 depth)
{
	u32 n;

	if ((mem[0x2842] & 1) == 0)
		return;

	for (n = 0; n < 256; n++)
///		if (mem[0x2c00 + 4*n]) {
//			if (mem[0x2c00 + 4*n] & 0xc000)
//				printf("sprite %04x %04x %04x %04x\n", mem[0x2c00 + 4*n],
//				       mem[0x2c01 + 4*n], mem[0x2c02 + 4*n], mem[0x2c03 + 4*n]);
			blit_sprite(depth, mem + 0x2c00 + 4*n);
///		}
}

void blit_screen(void)
{
//	printf("\033[H\033[2J\n\n");
#if 0
	printf("-----  VIDEO UPDATE  -----\n");

	printf("page 1:\n");
	printf("  x off  = %04x\n", mem[0x2810]);
	printf("  y off  = %04x\n", mem[0x2811]);
	printf("  attr   = %04x\n", mem[0x2812]);
	printf("  ctrl   = %04x\n", mem[0x2813]);	// 0x81: non-palette
	printf("  tiles  = %04x\n", mem[0x2814]);
	printf("  dunno5 = %04x\n", mem[0x2815]);
	printf("  bitmap = %04x\n", mem[0x2820]);

	printf("page 2:\n");
	printf("  x off  = %04x\n", mem[0x2816]);
	printf("  y off  = %04x\n", mem[0x2817]);
	printf("  attr   = %04x\n", mem[0x2818]);
	printf("  ctrl   = %04x\n", mem[0x2819]);
	printf("  tiles  = %04x\n", mem[0x281a]);
	printf("  dunno5 = %04x\n", mem[0x281b]);
	printf("  bitmap = %04x\n", mem[0x2821]);

	printf("sprites:\n");
	printf("  bitmap = %04x\n", mem[0x2822]);

	printf("\n");
#endif

	memset(screen, 0, sizeof screen);

	if (mem[0x3d20] & 0x0004)	// video DAC disable
		return;

	u32 depth;
	for (depth = 0; depth < 4; depth++) {
		if (!hide_page_1)
			blit_page(0, depth);
		if (!hide_page_2)
			blit_page(1, depth);
		if (!hide_sprites)
			blit_sprites(depth);
	}

	if (show_fps)
		do_show_fps();
}

void video_init(void)
{
	mem[0x2836] = 0xffff;
	mem[0x2837] = 0xffff;
}
