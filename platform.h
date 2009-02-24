// Copyright 2008  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#ifndef _PLATFORM_H
#define _PLATFORM_H

#include "types.h"

extern u8 controller_input[8];
extern u8 controller_output[7];
extern int controller_should_be_rotated;

extern u8 button_up;
extern u8 button_down;
extern u8 button_left;
extern u8 button_right;
extern u8 button_A;
extern u8 button_B;
extern u8 button_menu;

void platform_init(void);

extern void *rom_file;
void read_rom(u32 offset);

void update_screen(void);
char update_controller(void);

void fatal(const char *format, ...);

#endif
