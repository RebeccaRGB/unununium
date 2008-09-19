// Copyright 2008  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include <stdio.h>
#include <stdlib.h>

#include "types.h"
#include "emu.h"


u8 screen[320*240];


static const u8 sizes[] = { 8, 16, 32, 64 };
static const u8 colour_sizes[] = { 2, 4, 6, 8 };


static void blit(s32 xoff, s32 yoff, u16 flags, u16 *bitmap, u16 tile)
{
	u32 h = sizes[(flags & 0x00c0) >> 6];
	u32 w = sizes[(flags & 0x0030) >> 4];

	u32 yflipmask = flags & 0x0008 ? h - 1 : 0;
	u32 xflipmask = flags & 0x0004 ? w - 1 : 0;

	u32 nc = colour_sizes[flags & 0x0003];

	u32 palette_offset = (flags & 0x0f00) >> 4;

	u16 *m = bitmap + nc*w*h/16*tile;
	u32 bits = 0;
	u32 nbits = 0;

	for (u32 y = 0; y < h; y++) {
		u32 yy = yoff + (y ^ yflipmask);

		for (u32 x = 0; x < w; x++) {
			u32 xx = xoff + (x ^ xflipmask);

			bits <<= nc;
			if (nbits < nc) {
				u16 b = *m++;
				b = (b << 8) | (b >> 8);
				bits |= b << (nc - nbits);
				nbits += 16;
			}
			nbits -= nc;

			u32 pal = palette_offset + (bits >> 16);
			bits &= 0xffff;

			if (xx < 320 && yy < 240)
				if ((mem[0x2b00 + pal] & 0x8000) == 0)
					screen[xx + 320*yy] = pal;
		}
	}
}

static void blit_page(u32 depth, u32 bitmap, u16 *regs)
{
	u32 x0, y0;
	u32 yscroll = regs[1];
	u32 flags = regs[2];
	u32 flags2 = regs[3];
	u32 tilemap = regs[4];

	if ((flags2 & 8) == 0)
		return;

	if ((flags & 0x3000) >> 12 != depth)
		return;

	if ((flags & 0xf0) != 0x50) {
		printf("Background page flags %04x unhandled, QUIT\n", flags);
		exit(1);
	}

	for (y0 = 0; y0 < 16; y0++)
		for (x0 = 0; x0 < 32; x0++) {
			u16 tile = mem[tilemap + x0 + 32*y0];

			if (tile == 0)
				continue;

			u32 yy = ((16*y0 - yscroll + 0x10) & 0xff) - 0x10;
			blit(16*x0, yy, flags, mem+bitmap, tile);
		}
}

static void blit_sprite(u32 depth, u16 *sprite)
{
	u16 tile, flags;
	s16 x, y;
	u32 bitmap = 0x40*mem[0x2822];
	u32 w, h;

	tile = *sprite++;
	x = 160 + *sprite++;
	y = 120 - *sprite++;
	flags = *sprite++;

//if (flags & 0x8000) return;	// dunno
//if (flags & 0x4000) return;	// dunno

	if ((u32)(flags & 0x3000) >> 12 != depth)
		return;

	h = sizes[(flags & 0x00c0) >> 6];
	w = sizes[(flags & 0x0030) >> 4];

x -= (w/2);
y -= (h/2);
y += 8;

	blit(x, y, flags, mem+bitmap, tile);
}

static void blit_sprites(u32 depth)
{
	u32 n;
	for (n = 0; n < 256; n++)
		if (mem[0x2c00 + 4*n]) {
			if (mem[0x2c00 + 4*n] & 0xc000)
				printf("sprite %04x %04x %04x %04x\n", mem[0x2c00 + 4*n],
				       mem[0x2c01 + 4*n], mem[0x2c02 + 4*n], mem[0x2c03 + 4*n]);
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
	printf("  flags2 = %04x\n", mem[0x2813]);
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


	u32 depth;
	for (depth = 0; depth < 4; depth++) {
		blit_page(depth, 0x40*mem[0x2820], mem + 0x2810);
		blit_page(depth, 0x40*mem[0x2821], mem + 0x2816);
		blit_sprites(depth);
	}
}