#include <stdlib.h>

#include "types.h"
#include "sdl.h"


static SDL_Surface *screen;


static void ttt(int x, int y)
{
	u32 color = SDL_MapRGB(screen->format, 250, 150, 100);

	u16 *bufp;

	bufp = (u16 *)screen->pixels + y*screen->pitch/2 + x;
	*bufp = color;
}

void sdl_init(void)
{
	if (SDL_Init(SDL_INIT_AUDIO|SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
		exit(1);
	}
	atexit(SDL_Quit);

	screen = SDL_SetVideoMode(320, 240, 16, SDL_SWSURFACE);
	if (!screen) {
		fprintf(stderr, "Unable to set 320x240 video: %s\n", SDL_GetError());
		exit(1);
	}

	{
		int x, y;

		if (SDL_MUSTLOCK(screen))
			if (SDL_LockSurface(screen) < 0) {
				printf("oh crap.\n");
				exit(1);
			}

		for (y = 0; y < 240; y++)
			for (x = 0; x < 320; x++)
				ttt(x, y);

		if (SDL_MUSTLOCK(screen))
			SDL_UnlockSurface(screen);

		SDL_UpdateRect(screen, 0, 0, 320, 240);
	}
}

static u8 x58(u32 x)
{
	x &= 31;
	x *= 33;
	return x / 4;
}

void update_screen(u16 *mem)
{
	printf("\n\n");
	printf("-----  VIDEO UPDATE  -----\n");

	printf("page 0:\n");
	printf("  dunno0 = %04x\n", mem[0x2810]);
	printf("  dunno1 = %04x\n", mem[0x2811]);
	printf("  mode   = %04x\n", mem[0x2812]);
	printf("  flags  = %04x\n", mem[0x2813]);
	printf("  tiles  = %04x\n", mem[0x2814]);
	printf("  dunno5 = %04x\n", mem[0x2815]);
	printf("  bitmap = %04x\n", mem[0x2820]);

	printf("page 1:\n");
	printf("  dunno0 = %04x\n", mem[0x2816]);
	printf("  dunno1 = %04x\n", mem[0x2817]);
	printf("  mode   = %04x\n", mem[0x2818]);
	printf("  flags  = %04x\n", mem[0x2819]);
	printf("  tiles  = %04x\n", mem[0x281a]);
	printf("  dunno5 = %04x\n", mem[0x281b]);
	printf("  bitmap = %04x\n", mem[0x2821]);

	printf("sprites:\n");
	printf("  bitmap = %04x\n", mem[0x2822]);

	printf("\n");

	u32 palette[256];
	u32 n;
	for (n = 0; n < 256; n++) {
		u32 c = mem[0x2b00 + n];
		if (c & 0x8000)
			c = -1;
		else
			c = SDL_MapRGB(screen->format, x58(c >> 10), x58(c >> 5), x58(c));
		palette[n] = c;
	}

	if (SDL_MUSTLOCK(screen))
		if (SDL_LockSurface(screen) < 0) {
			printf("oh crap.\n");
			exit(1);
		}

	u32 x0, y0, x1, y1;
	u32 bitmap = 0x40*mem[0x2820];
	u32 tilemap = mem[0x2814];

	for (y0 = 0; y0 < 15; y0++)
		for (x0 = 0; x0 < 20; x0++) {
			u16 tile = mem[tilemap + x0 + 32*y0];

			if (tile == 0)
				continue;

			for (y1 = 0; y1 < 16; y1++)
				for (x1 = 0; x1 < 16; x1++) {
					u32 c;

					c = mem[bitmap + 128*tile + (16*y1 + x1)/2];
					if (x1 & 1)
						c >>= 8;
					else
						c &= 0xff;

					c = palette[c];

					if (c == -1)
						continue;

					u16 *buf = (u16 *)screen->pixels + (16*y0 + y1)*screen->pitch/2 + (16*x0 + x1);
					*buf = c;
				}
		}

	if (SDL_MUSTLOCK(screen))
		SDL_UnlockSurface(screen);

	SDL_UpdateRect(screen, 0, 0, 320, 240);
}
