// Copyright 2008-2010  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include <stdio.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <SDL.h>

#include "types.h"
#include "emu.h"
#include "video.h"
#include "audio.h"

#include "platform.h"


//#define SIZE_1X1
//#define SIZE_2X2
#define SIZE_3X3


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

void update_screen(void)
{
	blit_screen();

	if (SDL_MUSTLOCK(sdl_surface))
		if (SDL_LockSurface(sdl_surface) < 0)
			fatal("oh crap\n");

	u32 x, y;
	for (y = 0; y < 240; y++) {
#ifdef SIZE_1X1
		u32 *p = sdl_surface->pixels + y*sdl_surface->pitch;
		u8 *s_r = screen_r + 320*y;
		u8 *s_g = screen_g + 320*y;
		u8 *s_b = screen_b + 320*y;

		for (x = 0; x < 320; x++)
			*p++ = SDL_MapRGB(sdl_surface->format, *s_r++, *s_g++, *s_b++);
#endif
#ifdef SIZE_2X2
		u32 *p = sdl_surface->pixels + 2*y*sdl_surface->pitch;
		u32 *p2 = sdl_surface->pixels + (2*y+1)*sdl_surface->pitch;
		u8 *s_r = screen_r + 320*y;
		u8 *s_g = screen_g + 320*y;
		u8 *s_b = screen_b + 320*y;

		for (x = 0; x < 320; x++) {
			u32 c = SDL_MapRGB(sdl_surface->format, *s_r++, *s_g++, *s_b++);

			p[0] = c;
			p[1] = c;
			p += 2;

			p2[0] = c;
			p2[1] = c;
			p2 += 2;
		}
#endif
#ifdef SIZE_3X3
		u32 *p = sdl_surface->pixels + 3*y*sdl_surface->pitch;
		u32 *p2 = sdl_surface->pixels + (3*y+1)*sdl_surface->pitch;
		u32 *p3 = sdl_surface->pixels + (3*y+2)*sdl_surface->pitch;
		u8 *s_r = screen_r + 320*y;
		u8 *s_g = screen_g + 320*y;
		u8 *s_b = screen_b + 320*y;

		for (x = 0; x < 320; x++) {
			u32 c = SDL_MapRGB(sdl_surface->format, *s_r++, *s_g++, *s_b++);

			p[0] = c;
			p[1] = c;
			p[2] = c;
			p += 3;

			p2[0] = c;
			p2[1] = c;
			p2[2] = c;
			p2 += 3;

			p3[0] = c;
			p3[1] = c;
			p3[2] = c;
			p3 += 3;
		}
#endif
	}

	if (SDL_MUSTLOCK(sdl_surface))
		SDL_UnlockSurface(sdl_surface);

	SDL_UpdateRect(sdl_surface, 0, 0, 0, 0);
}


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
			fprintf(stderr, "Unknown SDL event type %d\n", event.type);
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

void fatal(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	vfprintf(stderr, format, ap);
	exit(1);
}


static void mix(void *cookie, u8 *data, int n)
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

#ifdef SIZE_1X1
	sdl_surface = SDL_SetVideoMode(320, 240, 32, SDL_SWSURFACE);
#endif
#ifdef SIZE_2X2
	sdl_surface = SDL_SetVideoMode(640, 480, 32, SDL_SWSURFACE);
#endif
#ifdef SIZE_3X3
	sdl_surface = SDL_SetVideoMode(960, 720, 32, SDL_SWSURFACE);
#endif
	if (!sdl_surface)
		fatal("Unable to initialise video: %s\n", SDL_GetError());

	SDL_AudioSpec spec = {
		.freq = 44100,
		.format = AUDIO_S16SYS,
		.channels = 1,
		.samples = 1024,
		.callback = mix,
		.userdata = 0
	}, actual;
	int ret = SDL_OpenAudio(&spec, &actual);
	printf("ret: %d (%s)\n", ret, SDL_GetError());
printf("--> fr=%d ch=%d sam=%d size=%d sil=x'%04x\n", actual.freq, actual.channels, actual.samples, actual.size, actual.silence);
	SDL_PauseAudio(0);

	// First SDL input does a lot of init, do it now.
	update_controller();
}
