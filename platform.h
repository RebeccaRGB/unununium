// Copyright 2008  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#ifndef _PLATFORM_H
#define _PLATFORM_H

#include "types.h"

extern u8 controller_input[8];
extern u8 controller_output[7];
extern int controller_should_be_rotated;

void platform_init(void);
void update_screen(void);
char update_controller(void);

#endif
