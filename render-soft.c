// Copyright 2008-2010  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

//#include <stdio.h>
#include <string.h>

#include "types.h"
#include "emu.h"
#include "board.h"
#include "video.h"
#include "platform.h"

#include "render.h"


u32 pixel_mask[3];
u32 pixel_shift[3];

static inline u8 mix_channel(u8 old, u8 new, u8 alpha)
{
	return ((255 - alpha)*old + alpha*new) / 255;
}

static inline void mix_pixel(u32 offset, u32 rgb, u8 alpha)
{
	u32 x = 0;
	u32 i;
	for (i = 0; i < 3; i++) {
		u8 old = (screen[offset] & pixel_mask[i]) >> pixel_shift[i];
		u8 new = (rgb & pixel_mask[i]) >> pixel_shift[i];
		x |= (mix_channel(old, new, alpha) << pixel_shift[i]) & pixel_mask[i];
	}
	screen[offset] = x;
}

void render_texture(int xoff, int yoff, int w, int h, u16 atlas_x, u16 atlas_y, u8 pal_offset, u8 alpha)
{
//debug("drawing at %d %d\n", atlas_x, atlas_y);
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
						mix_pixel(xx + 320*yy, palette_rgb[pal], alpha);
					else
						screen[xx + 320*yy] = palette_rgb[pal];
				}
			}
		}
	}
}

void render_rect(int x, int y, int w, int h, u8 r, u8 g, u8 b)
{
	u32 rgb = get_colour(r, g, b);

	u32 xx, yy;
	for (yy = y; yy < (u32)y + h; yy++)
		for (xx = x; xx < (u32)x + w; xx++)
			screen[320*yy + xx] = rgb;
}

void render_atlas(u32 atlas_y, u32 atlas_h)
{
	debug("atlas: %d+%d\n", atlas_y, atlas_h);
}


static void render_page_tile(u32 xoff, u32 yoff, u32 h, u32 w, u16 atlas_x, u16 atlas_y, u8 pal_offset, u8 alpha)
{
	for (u32 y = 0; y < h; y++) {
		int yy = yoff + y;
		yy &= 0x01ff;
		if (yy >= 0x01c0)
			yy -= 0x0200;

		int xx = xoff;
		if (yy < 0 || yy >= 240)
			continue;

		xx -= render_page_hoff[yy];

		xx &= 0x01ff;
		if (xx >= 0x01c0)
			xx -= 0x0200;

		if (xx + w <= 0 || xx >= 320)
			continue;

		render_texture(xx, yy, w, 1, atlas_x, atlas_y + y, pal_offset, alpha);
	}
}

void render_page(u32 xscroll, u32 yscroll, u32 h, u32 w)
{
	u32 hn = 256/h;
	u32 wn = 512/w;

	u32 x0, y0;
	for (y0 = 0; y0 < hn; y0++)
		for (x0 = 0; x0 < wn; x0++) {
			u32 index = x0 + wn*y0;

			u32 y = ((h*y0 - yscroll + 0x10) & 0xff) - 0x10;
			u32 x = (w*x0 - xscroll) & 0x1ff;

			u16 atlas_x = render_page_atlas_coors[2*index];
			u16 atlas_y = render_page_atlas_coors[2*index+1];
			u8 pal_offset = render_page_tile_colours[2*index];
			u8 alpha = render_page_tile_colours[2*index+1];

			render_page_tile(x, y, h, w, atlas_x, atlas_y, pal_offset, alpha);
		}
}

void render_begin(void)
{
	memset(screen, 0, sizeof screen);
}

void render_end(void)
{
}

void render_init(__unused u32 pixel_size)
{
}
