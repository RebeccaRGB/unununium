// Copyright 2008-2010  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#ifndef _VIDEO_H
#define _VIDEO_H

#include "types.h"

extern u32 palette_rgb[256];
extern u32 screen[320*240];

extern int hide_page_1;
extern int hide_page_2;
extern int hide_sprites;
extern int show_fps;


void video_store(u16 val, u32 addr);
u16 video_load(u32 addr);

void render_bitmap(u8 *dest, u32 pitch, u32 addr, u16 attr);

void video_init(void);

#endif
