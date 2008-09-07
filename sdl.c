#include <stdlib.h>
#include <sys/time.h>

#include "types.h"
#include "sdl.h"


static SDL_Surface *surface;
static u16 *screen;
static u32 pitch;


static u8 x58(u32 x)
{
	x &= 31;
	x *= 33;
	return x / 4;
}

static void blit_16x16x256(u16 *dest, u16 *mem, u32 bitmap, u16 tile, u32 *palette)
{
	u32 x, y;
	u16 *m = mem + bitmap + 128*tile;

	for (y = 0; y < 16; y++) {
		u16 *p = dest + pitch*y;

		for (x = 0; x < 8; x++) {
			u16 b;
			u32 c;

			b = *m++;

			c = palette[b & 0xff];
			if (c == (u32)-1)
				c = *p;
			*p++ = c;

			c = palette[b >> 8];
			if (c == (u32)-1)
				c = *p;
			*p++ = c;
		}
	}
}

static void blit_page_16x16x64(u16 *mem, u32 bitmap, u32 tilemap, u32 *palette)
{
	u32 x0, y0, x1, y1;

	for (y0 = 0; y0 < 15; y0++)
		for (x0 = 0; x0 < 20; x0++) {
			u16 tile = mem[tilemap + x0 + 32*y0];
			u16 *buf = screen + 16*pitch*y0 + 16*x0;

			if (tile == 0)
				continue;

			for (y1 = 0; y1 < 16; y1++)
				for (x1 = 0; x1 < 2; x1++) {
					u32 c;
					u32 o = bitmap + 96*tile + 6*y1 + 3*x1;

					u16 b0 = mem[o];
					u16 b1 = mem[o+1];
					u16 b2 = mem[o+2];
					b0 = (b0 << 8) | (b0 >> 8);
					b1 = (b1 << 8) | (b1 >> 8);
					b0 = (b2 << 8) | (b2 >> 8);

					c = b0 >> 10;
					c = palette[c];
					if (c == (u32)-1)
						continue;
					buf[pitch*y1 + 8*x1] = c;

					c = (b0 >> 4) & 0x3f;
					c = palette[c];
					if (c == (u32)-1)
						continue;
					buf[pitch*y1 + 8*x1 + 1] = c;

					c = ((b0 << 2) & 0x3c) | (b1 >> 14);
					c = palette[c];
					if (c == (u32)-1)
						continue;
					buf[pitch*y1 + 8*x1 + 2] = c;

					c = (b1 >> 8) & 0x3f;
					c = palette[c];
					if (c == (u32)-1)
						continue;
					buf[pitch*y1 + 8*x1 + 3] = c;

					c = (b1 >> 2) & 0x3f;
					c = palette[c];
					if (c == (u32)-1)
						continue;
					buf[pitch*y1 + 8*x1 + 4] = c;

					c = ((b1 << 4) & 0x30) | (b2 >> 12);
					c = palette[c];
					if (c == (u32)-1)
						continue;
					buf[pitch*y1 + 8*x1 + 5] = c;

					c = (b2 >> 6) & 0x3f;
					c = palette[c];
					if (c == (u32)-1)
						continue;
					buf[pitch*y1 + 8*x1 + 6] = c;

					c = b2 & 0x3f;
					c = palette[c];
					if (c == (u32)-1)
						continue;
					buf[pitch*y1 + 8*x1 + 7] = c;
				}
		}
}

static void blit_page_16x16x256(u16 *mem, u32 bitmap, u32 tilemap, u32 *palette)
{
	u32 x0, y0;

	for (y0 = 0; y0 < 15; y0++)
		for (x0 = 0; x0 < 20; x0++) {
			u16 tile = mem[tilemap + x0 + 32*y0];
			u16 *dest = screen + 16*pitch*y0 + 16*x0;

			if (tile == 0)
				continue;

			blit_16x16x256(dest, mem, bitmap, tile, palette);
		}
}

static void blit_page(u16 *mem, u32 page, u32 bitmap, u32 tilemap, u32 mode,
               u32 *palette)
{
	switch (mode & 0xff) {
		case 0x52:
			blit_page_16x16x64(mem, bitmap, tilemap, palette);
			break;
		case 0x53:
			blit_page_16x16x256(mem, bitmap, tilemap, palette);
			break;
		default:
			fprintf(stderr, "page %u mode unexpected: %04x\n",
			        page, mode);
	}
}

void update_screen(u16 *mem)
{
	static u32 last = 0;
	u32 now;
	struct timeval tv;

	gettimeofday(&tv, 0);
	now = 1000000*tv.tv_sec + tv.tv_usec;
	if (now < last + 40000)	// 25 FPS
		return;
	last = now;

	printf("\033[H\033[2J\n\n");
	printf("-----  VIDEO UPDATE  -----\n");

//	printf("page 0:\n");
//	printf("  dunno0 = %04x\n", mem[0x2810]);
//	printf("  dunno1 = %04x\n", mem[0x2811]);
//	printf("  mode   = %04x\n", mem[0x2812]);
//	printf("  flags  = %04x\n", mem[0x2813]);
//	printf("  tiles  = %04x\n", mem[0x2814]);
//	printf("  dunno5 = %04x\n", mem[0x2815]);
//	printf("  bitmap = %04x\n", mem[0x2820]);

//	printf("page 1:\n");
//	printf("  dunno0 = %04x\n", mem[0x2816]);
//	printf("  dunno1 = %04x\n", mem[0x2817]);
//	printf("  mode   = %04x\n", mem[0x2818]);
//	printf("  flags  = %04x\n", mem[0x2819]);
//	printf("  tiles  = %04x\n", mem[0x281a]);
//	printf("  dunno5 = %04x\n", mem[0x281b]);
//	printf("  bitmap = %04x\n", mem[0x2821]);

//	printf("sprites:\n");
//	printf("  bitmap = %04x\n", mem[0x2822]);

//	printf("\n");

	u32 palette[256];
	u32 n;
	for (n = 0; n < 256; n++) {
		u32 c = mem[0x2b00 + n];
		if (c & 0x8000)
			c = -1;
		else
			c = SDL_MapRGB(surface->format, x58(c >> 10), x58(c >> 5), x58(c));
		palette[n] = c;
	}

	if (SDL_MUSTLOCK(surface))
		if (SDL_LockSurface(surface) < 0) {
			printf("oh crap.\n");
			exit(1);
		}

	blit_page(mem, 0, 0x40*mem[0x2820], mem[0x2814], mem[0x2812], palette);
	blit_page(mem, 1, 0x40*mem[0x2821], mem[0x281a], mem[0x2818], palette);

	if (SDL_MUSTLOCK(surface))
		SDL_UnlockSurface(surface);

	SDL_UpdateRect(surface, 0, 0, 320, 240);
}

void sdl_init(void)
{
	if (SDL_Init(SDL_INIT_AUDIO|SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
		exit(1);
	}
	atexit(SDL_Quit);

	surface = SDL_SetVideoMode(320, 240, 16, SDL_SWSURFACE);
	if (!surface) {
		fprintf(stderr, "Unable to set 320x240 video: %s\n", SDL_GetError());
		exit(1);
	}

	screen = surface->pixels;
	pitch = surface->pitch / 2;
}
