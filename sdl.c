#include <stdlib.h>
#include <sys/time.h>

#include "types.h"
#include "sdl.h"


static const u8 sizes[] = { 8, 16, 32, 64 };

static SDL_Surface *surface;
static u16 *screen;
static u32 pitch;


static void blit_16(u16 *dest, u32 w, u32 h, u16 *mem, u32 bitmap, u16 tile, u32 *palette)
{
	u32 x, y;
	u16 *m = mem + bitmap + w*h/4*tile;

	for (y = 0; y < h; y++) {
		u16 *p = dest + pitch*y;

		for (x = 0; x < w; x += 4) {
			u16 b;
			u32 c;

			b = *m++;

			c = palette[(b >> 4) & 0x0f];
			if (c == (u32)-1)
				c = *p;
			*p++ = c;

			c = palette[b & 0x0f];
			if (c == (u32)-1)
				c = *p;
			*p++ = c;

			c = palette[(b >> 12) & 0x0f];
			if (c == (u32)-1)
				c = *p;
			*p++ = c;

			c = palette[(b >> 8) & 0x0f];
			if (c == (u32)-1)
				c = *p;
			*p++ = c;
		}
	}
}

static void blit_64(u16 *dest, u32 w, u32 h, u16 *mem, u32 bitmap, u16 tile, u32 *palette)
{
	u32 x, y;
	u16 *m = mem + bitmap + 3*w*h/8*tile;

	for (y = 0; y < h; y++) {
		u16 *p = dest + pitch*y;

		for (x = 0; x < w; x += 8) {
			u16 b;
			u32 c;

			u16 b0 = *m++;
			u16 b1 = *m++;
			u16 b2 = *m++;
			b0 = (b0 << 8) | (b0 >> 8);
			b1 = (b1 << 8) | (b1 >> 8);
			b2 = (b2 << 8) | (b2 >> 8);

			b = b0 >> 10;
			c = palette[b];
			if (c == (u32)-1)
				c = *p;
			*p++ = c;

			b = (b0 >> 4) & 0x3f;
			c = palette[b];
			if (c == (u32)-1)
				c = *p;
			*p++ = c;

			b = ((b0 << 2) & 0x3c) | (b1 >> 14);
			c = palette[b];
			if (c == (u32)-1)
				c = *p;
			*p++ = c;

			b = (b1 >> 8) & 0x3f;
			c = palette[b];
			if (c == (u32)-1)
				c = *p;
			*p++ = c;

			b = (b1 >> 2) & 0x3f;
			c = palette[b];
			if (c == (u32)-1)
				c = *p;
			*p++ = c;

			b = ((b1 << 4) & 0x30) | (b2 >> 12);
			c = palette[b];
			if (c == (u32)-1)
				c = *p;
			*p++ = c;

			b = (b2 >> 6) & 0x3f;
			c = palette[b];
			if (c == (u32)-1)
				c = *p;
			*p++ = c;

			b = b2 & 0x3f;
			c = palette[b];
			if (c == (u32)-1)
				c = *p;
			*p++ = c;
		}
	}
}

static void blit_256(u16 *dest, u32 w, u32 h, u16 *mem, u32 bitmap, u16 tile, u32 *palette)
{
	u32 x, y;
	u16 *m = mem + bitmap + w*h/2*tile;

	for (y = 0; y < h; y++) {
		u16 *p = dest + pitch*y;

		for (x = 0; x < w; x += 2) {
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
	u32 x0, y0;

	for (y0 = 0; y0 < 15; y0++)
		for (x0 = 0; x0 < 20; x0++) {
			u16 tile = mem[tilemap + x0 + 32*y0];
			u16 *dest = screen + 16*pitch*y0 + 16*x0;

			if (tile == 0)
				continue;

			blit_64(dest, 16, 16, mem, bitmap, tile, palette);
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

			blit_256(dest, 16, 16, mem, bitmap, tile, palette);
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

static void blit_sprite(u16 *mem, u16 *sprite, u32 *palette)
{
	u16 tile, x, y, flags;
	u16 *dest;
	u32 bitmap = 0x40*mem[0x2822];
	u32 w, h;

	tile = *sprite++;
	x = 160 - 16 + *sprite++;
	y = 120 - *sprite++;
	flags = *sprite++;

//if (flags & 0x8000) return;	// dunno
//if (flags & 0x4000) return;	// dunno
//if (flags & 0x2000) return;	// depth
//if (flags & 0x1000) return;	// depth

	palette += (flags & 0x0f00) >> 4;

	h = sizes[(flags & 0x00c0) >> 6];
	w = sizes[(flags & 0x0030) >> 4];

	if (x + w > 320 || x >= 320 || y + h > 240 || y >= 240) {
		printf("*** CLIP\n");
		return;
	}

//if (flags & 0x0008) return;	// Y flip
//if (flags & 0x0004) return;	// X flip

	dest = screen + pitch*y + x;

	switch (flags & 3) {
	case 0:
		printf("DANGER WILL ROBINSON\n");	// 4-colour
		break;
	case 1:
		blit_16(dest, w, h, mem, bitmap, tile, palette);
		break;
	case 2:
		blit_64(dest, w, h, mem, bitmap, tile, palette);
		break;
	case 3:
		blit_256(dest, w, h, mem, bitmap, tile, palette);
		break;
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

	for (n = 0; n < 256; n++)
		if (mem[0x2c00 + 4*n]) {
			printf("sprite %04x %04x %04x %04x\n", mem[0x2c00 + 4*n],
			       mem[0x2c01 + 4*n], mem[0x2c02 + 4*n], mem[0x2c03 + 4*n]);
			blit_sprite(mem, mem + 0x2c00 + 4*n, palette);
		}

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
