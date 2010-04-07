// Copyright 2010  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#ifndef _RENDER_H
#define _RENDER_H

void render_init(u32 pixel_size);
void render(void);

void render_kill_cache(void);

void render_palette(void);
void render_atlas(u32 atlas_y, u32 atlas_h);
void render_begin(void);
void render_end(void);


extern u16 render_page_atlas_coors[4096];
extern u8 render_page_tile_colours[4096];
extern u16 render_page_hoff[256];

void render_page(u32 xscroll, u32 yscroll, u32 h, u32 w);


void render_texture(int x, int y, int w, int h, u16 atlas_x, u16 atlas_y, u8 pal_offset, u8 alpha);
void render_rect(int x, int y, int w, int h, u8 r, u8 g, u8 b);


// ATLAS_W and ATLAS_H should be a multiple of 64 and a power of two;
// We need 2*512*256 for the two video pages, and 256*64*64 for the sprites.
// We also require 64 lines of slack for how we do the update.

// Let's use a 2048x2048 texture, that's 4MB.
#define ATLAS_W 2048
#define ATLAS_H 2048

extern u8 atlas[ATLAS_W*ATLAS_H];

#endif
