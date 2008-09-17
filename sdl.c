// Copyright 2008  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include <stdio.h>

#include "types.h"
#include "emu.h"
#include "video.h"
#include "sdl.h"


static SDL_Surface *sdl_surface;
static u32 palette[256];


static u8 x58(u32 x)
{
	x &= 31;
	return (x << 3) | (x >> 2);
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
	blit_screen();
	set_palette();

	if (SDL_MUSTLOCK(sdl_surface))
		if (SDL_LockSurface(sdl_surface) < 0) {
			printf("oh crap.\n");
			exit(1);
		}

	u32 x, y;
	for (y = 0; y < 240; y++) {
		u32 *p = sdl_surface->pixels + 2*y*sdl_surface->pitch;
		u32 *p2 = sdl_surface->pixels + (2*y+1)*sdl_surface->pitch;
		u8 *s = screen + 320*y;

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
