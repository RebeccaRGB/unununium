// Cannot name this "_SDL_H", the stupid SDL headers claim that for itself.
#ifndef _MY_SDL_H
#define _MY_SDL_H

// Include this here, since it redefines what main() means.  No taste.
#include <SDL.h>

#include "types.h"

void sdl_init(void);
void update_screen(u16 *mem);

#endif
