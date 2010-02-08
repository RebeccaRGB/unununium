// Copyright 2008,2009  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#ifndef _PLATFORM_H
#define _PLATFORM_H

#include "types.h"


extern u8 button_up;
extern u8 button_down;
extern u8 button_left;
extern u8 button_right;
extern u8 button_A;
extern u8 button_B;
extern u8 button_C;
extern u8 button_menu;

void platform_init(void);

void open_rom(const char *path);
void read_rom(u32 offset);

void *open_eeprom(const char *name, u8 *data, u32 len);
void save_eeprom(void *cookie, u8 *data, u32 len);

void update_screen(void);
char update_controller(void);

void fatal(const char *format, ...);
void warn(const char *format, ...);

#endif
