#include <stdlib.h>

#include "types.h"
#include "sdl.h"


static SDL_Surface *screen;


static void ttt(int x, int y)
{
	u32 color = SDL_MapRGB(screen->format, 200, 150, 100);

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
