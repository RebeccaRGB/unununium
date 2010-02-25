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
void render_begin(void); // XXX: get rid of this?
void render_end(void);

#define N_RENDER_TILES (2*256*32+256)
#define N_RENDER_RECTS 256

extern struct render_tile {
	int x, y, w, h;
	u32 atlas_x, atlas_y;
	u8 pal_offset, alpha;
} render_tiles[N_RENDER_TILES];

extern struct render_rect {
	int x, y, w, h;
	u8 r, g, b;
} render_rects[N_RENDER_RECTS];

extern u32 n_render_tiles, n_render_rects;


// ATLAS_W and ATLAS_H should be a multiple of 64 and a power of two;
// We need 2*512*256 for the two video pages, and 256*64*64 for the sprites.
// We also require 64 lines of slack for how we do the update.

// Let's use a 2048x2048 texture, that's 4MB.
#define ATLAS_W 2048
#define ATLAS_H 2048

extern u8 atlas[ATLAS_W*ATLAS_H];

#endif
