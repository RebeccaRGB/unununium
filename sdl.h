// Copyright 2008  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

// Cannot name this "_SDL_H", the stupid SDL header claims that for itself.
#ifndef _MY_SDL_H
#define _MY_SDL_H

#include "types.h"

extern u8 controller_input[8];
extern u8 controller_output[7];

void sdl_init(void);
void update_screen(void);
char update_controller(void);

#endif
