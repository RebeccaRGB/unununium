// Copyright 2008-2010  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include <stdio.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <SDL.h>
#include <SDL_opengl.h>

#include "types.h"
#include "emu.h"
#include "video.h"
#include "audio.h"
#include "render.h"

#include "platform.h"


#define PIXEL_SIZE 3


static void *rom_file;

void open_rom(const char *path)
{
	rom_file = fopen(path, "rb");
	if (!rom_file)
		fatal("Cannot read ROM file %s\n", path);
}

void read_rom(u32 offset)
{
	u32 n;

	fseek(rom_file, 2*(offset + 0x4000), SEEK_SET);
	n = fread(mem + 0x4000, 2, N_MEM - 0x4000, rom_file);

// gross, but whatever.  one day i'll fix this, but not today
#ifdef _BIG_ENDIAN
	for (u32 i = 0x4000; i < n + 0x4000; i++)
		mem[i] = (mem[i] << 8) | (mem[i] >> 8);
#endif
}


void *open_eeprom(const char *name, u8 *data, u32 len)
{
	char path[256];
	char *home;
	FILE *fp;

	memset(data, 0, len);

	home = getenv("HOME");
	if (!home)
		fatal("cannot find HOME\n");
	snprintf(path, sizeof path, "%s/.unununium", home);
	mkdir(path, 0777);
	snprintf(path, sizeof path, "%s/.unununium/%s.eeprom", home, name);

	fp = fopen(path, "rb");
	if (fp) {
		fread(data, 1, len, fp);
		if (ferror(fp))
			fatal("error reading EEPROM file %s\n", path);
		fclose(fp);
	}

	fp = fopen(path, "wb");
	if (!fp)
		fatal("cannot open EEPROM file %s\n", path);

	fwrite(data, 1, len, fp);
	fflush(fp);

	return fp;
}

void save_eeprom(void *cookie, u8 *data, u32 len)
{
	FILE *fp = cookie;

	rewind(fp);

	fwrite(data, 1, len, fp);
	fflush(fp);
}


static SDL_Surface *sdl_surface;

#ifdef RENDER_soft
static inline u8 x58(u32 x)
{
	x &= 31;
	return (x << 3) | (x >> 2);
}

u32 get_colour(u8 r, u8 g, u8 b)
{
	return SDL_MapRGB(sdl_surface->format, r, g, b);
}

void render_palette(void)
{
	u32 i;
	for (i = 0; i < 256; i++) {
		u16 p = mem[0x2b00 + i];
		palette_rgb[i] = SDL_MapRGB(sdl_surface->format,
		                            x58((p >> 10) & 31),
		                            x58((p >> 5) & 31),
		                            x58(p & 31));
	}
}

void update_screen(void)
{
	if (SDL_MUSTLOCK(sdl_surface))
		if (SDL_LockSurface(sdl_surface) < 0)
			fatal("oh crap\n");

	u32 pitch = sdl_surface->pitch / 4;
	u32 *pixels = sdl_surface->pixels;

	u32 x, y, j;
	for (y = 0; y < 240; y++) {
		u32 *p = pixels + PIXEL_SIZE*y*pitch;
		u32 *s = screen + 320*y;

		for (x = 0; x < 320; x++) {
			u32 c = *s++;

			for (j = 0; j < PIXEL_SIZE; j++)
				p[PIXEL_SIZE*x + j] = c;
		}

		for (j = 1; j < PIXEL_SIZE; j++)
			memcpy(p + j*pitch, p, 4*PIXEL_SIZE*320);
	}

	if (SDL_MUSTLOCK(sdl_surface))
		SDL_UnlockSurface(sdl_surface);

	SDL_UpdateRect(sdl_surface, 0, 0, 0, 0);
}
#endif

#ifdef RENDER_gl
void update_screen(void)
{
	SDL_GL_SwapBuffers();
}
#endif


u8 button_up;
u8 button_down;
u8 button_left;
u8 button_right;
u8 button_A;
u8 button_B;
u8 button_C;
u8 button_menu;

static char handle_debug_key(int key)
{
	switch (key) {
	case SDLK_ESCAPE:
		return 0x1b;
	case '0' ... '8':
	case 't':
	case 'y':
	case 'u':
	case 'x':
	case 'q':
	case 'w': case 'e':
	case 'a': case 's': case 'd':
	case 'f':
	case 'v': case 'c':
		return (key);
	}

	return 0;
}

static void handle_controller_key(int key, int down)
{
	switch (key) {
	case SDLK_UP: case 'j':
		button_up = down;
		break;
	case SDLK_DOWN: case 'm':
		button_down = down;
		break;
	case SDLK_LEFT: case 'n':
		button_left = down;
		break;
	case SDLK_RIGHT: case ',':
		button_right = down;
		break;
	case SDLK_SPACE: case 'h':
		button_A = down;
		break;
	case 'b': case 'k':
		button_B = down;
		break;
	case 'g':
		button_C = down;
		break;
	case 'l':
		button_menu = down;
		break;
	}
}

char update_controller(void)
{
	SDL_Event event;
	char key;

	while (SDL_PollEvent(&event)) {

		switch (event.type) {
		case SDL_KEYDOWN:
			key = handle_debug_key(event.key.keysym.sym);
			if (key)
				return (key);

			// Fallthrough.
		case SDL_KEYUP:
			handle_controller_key(event.key.keysym.sym,
			                      (event.type == SDL_KEYDOWN));
			break;

		case SDL_QUIT:
			return 0x1b;

		case SDL_ACTIVEEVENT:
		case SDL_MOUSEMOTION:
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			continue;

		default:
			warn("Unknown SDL event type %d\n", event.type);
			continue;
		}
	}

	return 0;
}


u32 get_realtime(void)
{
	struct timeval tv;

	gettimeofday(&tv, 0);
	return 1000000*tv.tv_sec + tv.tv_usec;
}


void warn(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	vfprintf(stderr, format, ap);
}

#ifdef USE_DEBUG
void debug(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	vfprintf(stderr, format, ap);
}
#endif

void fatal(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	vfprintf(stderr, format, ap);
	exit(1);
}


static void mix(__unused void *cookie, u8 *data, int n)
{
	if (mute_audio) {
		memset(data, 0, n);
		return;
	}

	audio_render((s16 *)data, n/2);
}


void platform_init(void)
{
	if (SDL_Init(SDL_INIT_AUDIO|SDL_INIT_VIDEO) < 0)
		fatal("Unable to init SDL: %s\n", SDL_GetError());
	atexit(SDL_Quit);

	SDL_WM_SetCaption("Unununium", 0);

#ifdef RENDER_soft
	sdl_surface = SDL_SetVideoMode(PIXEL_SIZE*320, PIXEL_SIZE*240, 0,
	                               SDL_SWSURFACE);

	if (!sdl_surface)
		fatal("Unable to initialise video: %s\n", SDL_GetError());
	if (sdl_surface->format->BytesPerPixel != 4)
		fatal("Didn't get a 32-bit surface\n");

	pixel_mask[0] = sdl_surface->format->Rmask;
	pixel_mask[1] = sdl_surface->format->Gmask;
	pixel_mask[2] = sdl_surface->format->Bmask;
	pixel_shift[0] = sdl_surface->format->Rshift;
	pixel_shift[1] = sdl_surface->format->Gshift;
	pixel_shift[2] = sdl_surface->format->Bshift;
#endif

#ifdef RENDER_gl
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);

	sdl_surface = SDL_SetVideoMode(PIXEL_SIZE*320, PIXEL_SIZE*240, 0, SDL_OPENGL);

	if (!sdl_surface)
		fatal("Unable to initialise video: %s\n", SDL_GetError());
#endif

	render_kill_cache();
	render_init(PIXEL_SIZE);

	SDL_AudioSpec spec = {
		.freq = 44100,
		.format = AUDIO_S16SYS,
		.channels = 1,
		.samples = 1024,
		.callback = mix,
		.userdata = 0
	}, actual;
	int ret = SDL_OpenAudio(&spec, &actual);
	debug("ret: %d (%s)\n", ret, SDL_GetError());
debug("--> fr=%d ch=%d sam=%d size=%d sil=x'%04x\n", actual.freq, actual.channels, actual.samples, actual.size, actual.silence);
	SDL_PauseAudio(0);

	// First SDL input does a lot of init, do it now.
	update_controller();
}
