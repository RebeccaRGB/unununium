// Copyright 2008-2010  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

//#include <stdio.h>
#include <string.h>

#include "types.h"
#include "emu.h"
#include "board.h"
#include "video.h"

#include "render.h"


static const u8 sizes[] = { 8, 16, 32, 64 };

u32 pixel_mask[3];
u32 pixel_shift[3];

static inline u8 mix_channel(u8 old, u8 new)
{
	u8 alpha = mem[0x282a];
	return ((4 - alpha)*old + alpha*new) / 4;
}

static inline void mix_pixel(u32 offset, u32 rgb)
{
	u32 x = 0;
	u32 i;
	for (i = 0; i < 3; i++) {
		u8 old = (screen[offset] & pixel_mask[i]) >> pixel_shift[i];
		u8 new = (rgb & pixel_mask[i]) >> pixel_shift[i];
		x |= (mix_channel(old, new) << pixel_shift[i]) & pixel_mask[i];
	}
	screen[offset] = x;
}

static void do_render_texture(int xoff, int yoff, int w, int h, u32 atlas_x, u32 atlas_y, u8 pal_offset, u8 alpha)
{
//printf("drawing at %d %d\n", atlas_x, atlas_y);
	u8 *p = &atlas[atlas_y*ATLAS_W + atlas_x];

	for (u32 y = 0; y < (u32)h; y++) {
		u32 yy = (yoff + y) & 0x1ff;

		for (u32 x = 0; x < (u32)w; x++) {
			u32 xx = (xoff + x) & 0x1ff;

			u8 pal = p[y*ATLAS_W + x] + pal_offset;

			if (xx < 320 && yy < 240) {
				u16 rgb = mem[0x2b00 + pal];
				if ((rgb & 0x8000) == 0) {
					if (alpha != 255)
						mix_pixel(xx + 320*yy, palette_rgb[pal]);
					else
						screen[xx + 320*yy] = palette_rgb[pal];
				}
			}
		}
	}
}

static void do_render_rect(int x, int y, int w, int h, u8 r, u8 g, u8 b)
{
	u32 rgb = get_colour(r, g, b);

	u32 xx, yy;
	for (yy = y; yy < (u32)y + h; yy++)
		for (xx = x; xx < (u32)x + w; xx++)
			screen[320*yy + xx] = rgb;
}

void render_atlas(u32 atlas_y, u32 atlas_h)
{
	printf("atlas: %d+%d\n", atlas_y, atlas_h);
}

void __render_begin(void)
{
}

void render_end(void)
{
	memset(screen, 0, sizeof screen);

	u32 i;
	for (i = 0; i < n_render_tiles; i++) {
		struct render_tile *tile = &render_tiles[i];
		do_render_texture(tile->x, tile->y, tile->w, tile->h, tile->atlas_x, tile->atlas_y, tile->pal_offset, tile->alpha);
	}

	for (i = 0; i < n_render_rects; i++) {
		struct render_rect *rect = &render_rects[i];
		do_render_rect(rect->x, rect->y, rect->w, rect->h, rect->r, rect->g, rect->b);
	}
}

void render_init(u32 pixel_size)
{
}
