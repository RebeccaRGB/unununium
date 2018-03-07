// Copyright 2008-2010  Segher Boessenkool  <segher@kernel.crashing.org>
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
extern u8 button_red;
extern u8 button_yellow;
extern u8 button_blue;
extern u8 button_green;

void platform_init(void);

void open_rom(const char *path);
void read_rom(u32 offset);

void *open_eeprom(const char *name, u8 *data, u32 len);
void save_eeprom(void *cookie, u8 *data, u32 len);

extern u32 pixel_mask[3];
extern u32 pixel_shift[3];

void update_screen(void);
char update_controller(void);

u32 get_realtime(void);

void __noreturn fatal(const char *format, ...);
void warn(const char *format, ...);
#ifdef USE_DEBUG
void debug(const char *format, ...);
#else
static inline void debug(__unused const char *format, ...) { }
#endif

#endif
