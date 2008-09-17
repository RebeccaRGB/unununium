// Copyright 2008  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include <stdlib.h>

#include "types.h"
#include "emu.h"
#include "sdl.h"


static const u8 sizes[] = { 8, 16, 32, 64 };
static const u8 colour_sizes[] = { 2, 4, 6, 8 };

static SDL_Surface *sdl_surface;
static u8 screen[(320+128)*(240+128)];
static u32 pitch = 320+128;
static u32 palette[256];


static void blit(s32 xx, s32 yy, u16 flags, u16 *bitmap, u16 tile)
{
	u32 x, y, h, w, nc;
	u16 *m;
	u32 bits, nbits;
	u8 palette_offset;

	xx += 64;
	yy += 64;

	if ((u32)xx >= 320+64 || (u32)yy >= 240+64)
		return;

	u8 *dest = screen + (s32)(pitch*yy + xx);

	h = sizes[(flags & 0x00c0) >> 6];
	w = sizes[(flags & 0x0030) >> 4];

	u32 yflipmask = flags & 0x0008 ? h - 1 : 0;
	u32 xflipmask = flags & 0x0004 ? w - 1 : 0;

	nc = colour_sizes[flags & 0x0003];

	palette_offset = (flags & 0x0f00) >> 4;

	m = bitmap + nc*w*h/16*tile;
	bits = 0;
	nbits = 0;

	for (y = 0; y < h; y++) {
		u8 *p;

		p = dest + pitch*(y ^ yflipmask);

		for (x = 0; x < w; x++) {
			u16 b;
			u32 c;

			bits <<= nc;
			if (nbits < nc) {
				b = *m++;
				b = (b << 8) | (b >> 8);
				bits |= b << (nc - nbits);
				nbits += 16;
			}
			nbits -= nc;

			b = bits >> 16;
			bits &= 0xffff;

			c = palette[palette_offset + b];
			if (c != (u32)-1)
				p[x ^ xflipmask] = palette_offset + b;
		}
	}
}

static void blit_page(u32 depth, u32 bitmap, u16 *regs)
{
	u32 x0, y0;
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

	for (y0 = 0; y0 < 15; y0++)
		for (x0 = 0; x0 < 20; x0++) {
			u16 tile = mem[tilemap + x0 + 32*y0];

			if (tile == 0)
				continue;

			blit(16*x0, 16*y0, flags, mem+bitmap, tile);
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

static u8 x58(u32 x)
{
	x &= 31;
	x *= 33;
	return x / 4;
}

static void set_palette(void)
{
	u32 n;
	for (n = 0; n < 256; n++) {
		u32 c = mem[0x2b00 + n];
		if (c & 0x8000)
			c = -1;
		else
			c = SDL_MapRGB(sdl_surface->format, x58(c >> 10), x58(c >> 5), x58(c));
		palette[n] = c;
	}
}

void update_screen(void)
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

	set_palette();

	u32 depth;
	for (depth = 0; depth < 4; depth++) {
		blit_page(depth, 0x40*mem[0x2820], mem + 0x2810);
		blit_page(depth, 0x40*mem[0x2821], mem + 0x2816);
		blit_sprites(depth);
	}

	if (SDL_MUSTLOCK(sdl_surface))
		if (SDL_LockSurface(sdl_surface) < 0) {
			printf("oh crap.\n");
			exit(1);
		}

	u32 x, y;
	for (y = 0; y < 240; y++) {
		u32 *p = sdl_surface->pixels + 2*y*sdl_surface->pitch;
		u32 *p2 = sdl_surface->pixels + (2*y+1)*sdl_surface->pitch;
		u8 *s = screen + 64 + (64+y)*pitch;

		for (x = 0; x < 320; x += 4) {
			u32 c0 = s[0];
			u32 c1 = s[1];
			u32 c2 = s[2];
			u32 c3 = s[3];
			c0 = palette[c0];
			c1 = palette[c1];
			c2 = palette[c2];
			c3 = palette[c3];
			s += 4;

			p[0] = c0;
			p[1] = c0;
			p[2] = c1;
			p[3] = c1;
			p[4] = c2;
			p[5] = c2;
			p[6] = c3;
			p[7] = c3;
			p += 8;

			p2[0] = c0;
			p2[1] = c0;
			p2[2] = c1;
			p2[3] = c1;
			p2[4] = c2;
			p2[5] = c2;
			p2[6] = c3;
			p2[7] = c3;
			p2 += 8;
		}
	}

	if (SDL_MUSTLOCK(sdl_surface))
		SDL_UnlockSurface(sdl_surface);

	SDL_UpdateRect(sdl_surface, 0, 0, 0, 0);
}

void sdl_init(void)
{
	if (SDL_Init(SDL_INIT_AUDIO|SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
		exit(1);
	}
	atexit(SDL_Quit);

	SDL_WM_SetCaption("Unununium", 0);

	sdl_surface = SDL_SetVideoMode(640, 480, 32, SDL_SWSURFACE);
	if (!sdl_surface) {
		fprintf(stderr, "Unable to set 320x240 video: %s\n", SDL_GetError());
		exit(1);
	}
}
