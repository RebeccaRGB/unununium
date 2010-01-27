// Copyright 2008,2009  Segher Boessenkool  <segher@kernel.crashing.org>
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


u8 screen_r[320*240];
u8 screen_g[320*240];
u8 screen_b[320*240];

int hide_page_0;
int hide_page_1;
int hide_sprites;


static const u8 sizes[] = { 8, 16, 32, 64 };
static const u8 colour_sizes[] = { 2, 4, 6, 8 };


static void do_dma(u32 len)
{
	u32 src = mem[0x2870];
	u32 dst = mem[0x2871] + 0x2c00;
	u32 j;

	for (j = 0; j < len; j++)
		mem[dst+j] = mem[src+j];

	mem[0x2872] = 0;
}

void video_store(u16 val, u32 addr)
{
	if (addr < 0x2900) {		// video regs
		switch (addr) {
		case 0x2810 ... 0x2815:	// page 0 regs
		case 0x2816 ... 0x281b:	// page 1 regs
			break;

//		case 0x281c:		// blend level: 0=opaque, 40=transp
//			break;

		case 0x2820 ... 0x2822:	// bitmap offsets
			break;

//		case 0x282a:		// XXX
//			break;

//		case 0x2830:		// XXX
//			break;

//		case 0x2836:
//		case 0x2837:		// XXX
//			break;

//		case 0x283c:		// XXX
//			break;

//		case 0x2842:		// XXX
//			break;

		case 0x2862:		// video IRQ enable
			break;

		case 0x2863:		// video IRQ ACK
			mem[addr] &= ~val;
			if (val & 1)
				update_screen();
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
	} else if (addr < 0x2b00) {	// scroll per raster line
	} else if (addr < 0x2c00) {	// palette
	} else {			// sprites
	}

	mem[addr] = val;
}

u16 video_load(u32 addr)
{
	u16 val = mem[addr];

	if (addr < 0x2900) {
		switch (addr) {
		case 0x2810 ... 0x2815:	// page 0 regs
		case 0x2816 ... 0x281b:	// page 1 regs
			break;

		case 0x282a:		// XXX
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


static u8 x58(u32 x)
{
	x &= 31;
	return (x << 3) | (x >> 2);
}

static void set_pixel(u32 offset, u16 rgb)
{
	screen_r[offset] = x58(rgb >> 10);
	screen_g[offset] = x58(rgb >> 5);
	screen_b[offset] = x58(rgb);
}

static u8 mix_channel(u8 old, u8 new)
{
	u8 alpha = mem[0x281c];
	return ((64 - alpha)*old + alpha*new) / 64;
}

static void mix_pixel(u32 offset, u16 rgb)
{
	screen_r[offset] = mix_channel(screen_r[offset], x58(rgb >> 10));
	screen_g[offset] = mix_channel(screen_g[offset], x58(rgb >> 5));
	screen_b[offset] = mix_channel(screen_b[offset], x58(rgb));
}

static void blit(u32 xoff, u32 yoff, u32 flags, u32 bitmap, u16 tile)
{
	u32 h = sizes[(flags & 0x00c0) >> 6];
	u32 w = sizes[(flags & 0x0030) >> 4];

	u32 yflipmask = flags & 0x0008 ? h - 1 : 0;
	u32 xflipmask = flags & 0x0004 ? w - 1 : 0;

	u32 nc = colour_sizes[flags & 0x0003];

	u32 palette_offset = (flags & 0x0f00) >> 4;

	u32 m = bitmap + nc*w*h/16*tile;
	u32 bits = 0;
	u32 nbits = 0;

	for (u32 y = 0; y < h; y++) {
		u32 yy = (yoff + (y ^ yflipmask)) & 0x1ff;

		for (u32 x = 0; x < w; x++) {
			u32 xx = (xoff + (x ^ xflipmask)) & 0x1ff;

			bits <<= nc;
			if (nbits < nc) {
				u16 b = mem[m++ & 0x3fffff];
				b = (b << 8) | (b >> 8);
				bits |= b << (nc - nbits);
				nbits += 16;
			}
			nbits -= nc;

			u32 pal = palette_offset + (bits >> 16);
			bits &= 0xffff;

			if ((flags & 0x00100000) && yy < 240)
				xx = (xx - mem[0x2900 + yy]) & 0x01ff;

			//if ((flags & 0x00200000) && yy < 240)
			//	xx = (xx - mem[0x2a00 + yy]) & 0x01ff;

			if (xx < 320 && yy < 240) {
				u16 rgb = mem[0x2b00 + pal];
				if ((rgb & 0x8000) == 0) {
					if (flags & 0x4000)
						mix_pixel(xx + 320*yy, rgb);
					else
						set_pixel(xx + 320*yy, rgb);
				}
			}
		}
	}
}

static void blit_page(u32 depth, u32 bitmap, u16 *regs)
{
	u32 x0, y0;
	u32 xscroll = regs[0];
	u32 yscroll = regs[1];
	u32 flags = regs[2];
	u32 flags2 = regs[3];
	u32 tilemap = regs[4];
	u32 palette_map = regs[5];

	if ((flags2 & 8) == 0)
		return;

	if ((flags & 0x3000) >> 12 != depth)
		return;

	//if ((flags & 0x8000) | (flags2 & 0xfff7))
	//	printf("flags: %04x, flags2: %04x\n", flags, flags2);

	u32 h = sizes[(flags & 0x00c0) >> 6];
	u32 w = sizes[(flags & 0x0030) >> 4];

	u32 hn = 256/h;
	u32 wn = 512/h;

	for (y0 = 0; y0 < hn; y0++)
		for (x0 = 0; x0 < wn; x0++) {
			u16 tile = mem[tilemap + x0 + wn*y0];

			if (tile == 0)
				continue;

			u16 palette = mem[palette_map + (x0 + wn*y0)/2];
			if (x0 & 1)
				palette >>= 8;

			u32 tileflags = 0;
			if (palette & 0x10)
				tileflags |= 4;		// X flip
			tileflags |= (palette & 0x0f) << 8;

			u32 yy = ((h*y0 - yscroll + 0x10) & 0xff) - 0x10;
			u32 xx = (w*x0 - xscroll) & 0x1ff;

			blit(xx, yy, (flags2 << 16) | flags | tileflags, bitmap, tile);
		}
}

static void blit_sprite(u32 depth, u16 *sprite)
{
	u16 tile, flags;
	s16 x, y;
	u32 bitmap = 0x40*mem[0x2822];

	tile = sprite[0];
	x = sprite[1];
	y = sprite[2];
	flags = sprite[3];
if (tile >= 0x8000)
	printf("UH-OH: %04x %04x %04x %04x\n", tile, x, y, flags);

	if ((u32)(flags & 0x3000) >> 12 != depth)
		return;

	if (board->use_centered_coors) {
		x = 160 + x;
		y = 120 - y;

		u32 h = sizes[(flags & 0x00c0) >> 6];
		u32 w = sizes[(flags & 0x0030) >> 4];

		x -= (w/2);
		y -= (h/2) - 8;
	}

	x = x & 0x1ff;
	y = y & 0x1ff;

	blit(x, y, flags, bitmap, tile);
}

static void blit_sprites(u32 depth)
{
	u32 n;
	for (n = 0; n < 256; n++)
		if (mem[0x2c00 + 4*n]) {
//			if (mem[0x2c00 + 4*n] & 0xc000)
//				printf("sprite %04x %04x %04x %04x\n", mem[0x2c00 + 4*n],
//				       mem[0x2c01 + 4*n], mem[0x2c02 + 4*n], mem[0x2c03 + 4*n]);
			blit_sprite(depth, mem + 0x2c00 + 4*n);
		}
}

void blit_screen(void)
{
//	printf("\033[H\033[2J\n\n");
#if 0
	printf("-----  VIDEO UPDATE  -----\n");

	printf("page 0:\n");
	printf("  x off  = %04x\n", mem[0x2810]);
	printf("  y off  = %04x\n", mem[0x2811]);
	printf("  flags  = %04x\n", mem[0x2812]);
	printf("  flags2 = %04x\n", mem[0x2813]);	// 0x81: non-palette
	printf("  tiles  = %04x\n", mem[0x2814]);
	printf("  dunno5 = %04x\n", mem[0x2815]);
	printf("  bitmap = %04x\n", mem[0x2820]);

	printf("page 1:\n");
	printf("  x off  = %04x\n", mem[0x2816]);
	printf("  y off  = %04x\n", mem[0x2817]);
	printf("  flags  = %04x\n", mem[0x2818]);
	printf("  flags2 = %04x\n", mem[0x2819]);
	printf("  tiles  = %04x\n", mem[0x281a]);
	printf("  dunno5 = %04x\n", mem[0x281b]);
	printf("  bitmap = %04x\n", mem[0x2821]);

	printf("sprites:\n");
	printf("  bitmap = %04x\n", mem[0x2822]);

	printf("\n");
#endif

	// Could optimise this, skip if any full page shown
	memset(screen_r, 0, sizeof screen_r);
	memset(screen_g, 0, sizeof screen_g);
	memset(screen_b, 0, sizeof screen_b);

	u32 depth;
	for (depth = 0; depth < 4; depth++) {
		if (!hide_page_0)
			blit_page(depth, 0x40*mem[0x2820], mem + 0x2810);
		if (!hide_page_1)
			blit_page(depth, 0x40*mem[0x2821], mem + 0x2816);
		if (!hide_sprites)
			blit_sprites(depth);
	}
}
