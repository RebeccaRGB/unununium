#include <stdlib.h>
#include <sys/time.h>

#include "types.h"
#include "sdl.h"


static const u8 sizes[] = { 8, 16, 32, 64 };
static const u8 colour_sizes[] = { 2, 4, 6, 8 };

static SDL_Surface *surface;
static u16 *screen;
static u32 pitch;


static void blit(u16 *dest, u16 flags, u16 *mem, u32 bitmap, u16 tile, u32 *palette)
{
	u32 x, y, h, w, nc;
	u16 *m;
	u32 bits, nbits;

	h = sizes[(flags & 0x00c0) >> 6];
	w = sizes[(flags & 0x0030) >> 4];
	nc = colour_sizes[flags & 0x0003];

	m = mem + bitmap + nc*w*h/16*tile;
	bits = 0;
	nbits = 0;

	for (y = 0; y < h; y++) {
		u16 *p = dest + pitch*y;

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

			c = palette[b];
			if (c == (u32)-1)
				c = *p;
			*p++ = c;
		}
	}
}

static void blit_page(u16 *mem, u32 page, u32 bitmap, u32 tilemap, u32 flags, u32 *palette)
{
	u32 x0, y0;

	if (flags == 0) {	// shouldn't get here, but we generate IRQs too early
		printf("FIXME, early video IRQ\n");
		return;
	}

	if ((flags & 0xf0) != 0x50) {
		printf("Background page flags %04x unhandled, QUIT\n", flags);
		exit(1);
	}

	for (y0 = 0; y0 < 15; y0++)
		for (x0 = 0; x0 < 20; x0++) {
			u16 tile = mem[tilemap + x0 + 32*y0];
			u16 *dest = screen + 16*pitch*y0 + 16*x0;

			if (tile == 0)
				continue;

			blit(dest, flags, mem, bitmap, tile, palette);
		}
}

static void blit_sprite(u16 *mem, u16 *sprite, u32 *palette)
{
	u16 tile, flags;
	s16 x, y;
	u16 *dest;
	u32 bitmap = 0x40*mem[0x2822];
	u32 w, h;

	tile = *sprite++;
	x = 160 + *sprite++;
	y = 120 - *sprite++;
	flags = *sprite++;

//if (flags & 0x8000) return;	// dunno
//if (flags & 0x4000) return;	// dunno
//if (flags & 0x2000) return;	// depth
//if (flags & 0x1000) return;	// depth

	palette += (flags & 0x0f00) >> 4;

	h = sizes[(flags & 0x00c0) >> 6];
	w = sizes[(flags & 0x0030) >> 4];

x -= (w/2);
y -= (h/2);

	// These are unsigned, so they check for "< 0" as well.
	if (x + w > 320+128 || y + h > 240+128) {
		printf("*** CLIP\n");
		printf("x,y=%d,%d w,h=%d,%d\n", (int)x, (int)y, (int)w, (int)h);
		return;
	}

//if (flags & 0x0008) return;	// Y flip
//if (flags & 0x0004) return;	// X flip

	dest = screen + (s32)(pitch*y + x);

	blit(dest, flags, mem, bitmap, tile, palette);
}

static u8 x58(u32 x)
{
	x &= 31;
	x *= 33;
	return x / 4;
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
//	printf("  flags  = %04x\n", mem[0x2812]);
//	printf("  flags2 = %04x\n", mem[0x2813]);
//	printf("  tiles  = %04x\n", mem[0x2814]);
//	printf("  dunno5 = %04x\n", mem[0x2815]);
//	printf("  bitmap = %04x\n", mem[0x2820]);

//	printf("page 1:\n");
//	printf("  dunno0 = %04x\n", mem[0x2816]);
//	printf("  dunno1 = %04x\n", mem[0x2817]);
//	printf("  flags  = %04x\n", mem[0x2818]);
//	printf("  flags2 = %04x\n", mem[0x2819]);
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

	for (n = 0; n < 256; n++)
		if (mem[0x2c00 + 4*n]) {
//			printf("sprite %04x %04x %04x %04x\n", mem[0x2c00 + 4*n],
//			       mem[0x2c01 + 4*n], mem[0x2c02 + 4*n], mem[0x2c03 + 4*n]);
			blit_sprite(mem, mem + 0x2c00 + 4*n, palette);
		}

	if (SDL_MUSTLOCK(surface))
		SDL_UnlockSurface(surface);

	SDL_UpdateRect(surface, 0, 0, 320+128, 240+128);
}

void sdl_init(void)
{
	if (SDL_Init(SDL_INIT_AUDIO|SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
		exit(1);
	}
	atexit(SDL_Quit);

	surface = SDL_SetVideoMode(320+128, 240+128, 16, SDL_SWSURFACE);
	if (!surface) {
		fprintf(stderr, "Unable to set 320x240 video: %s\n", SDL_GetError());
		exit(1);
	}

	screen = surface->pixels;
	pitch = surface->pitch / 2;

	screen += 64*(pitch + 1);
}
