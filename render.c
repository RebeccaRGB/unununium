// Copyright 2008-2010  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include "types.h"
#include "emu.h"
#include "video.h"
#include "board.h"	// for use_centered_coors
#include "platform.h"

#include "render.h"


u8 atlas[ATLAS_W*ATLAS_H];
static u32 atlas_x, atlas_y, atlas_h, atlas_low_x, atlas_low_y;


#define HASH_SIZE 65536
#define CACHE_SIZE (HASH_SIZE + ATLAS_W*ATLAS_H/64)
static struct cache {
	u32 key;
	struct cache *next;
	u32 atlas_x, atlas_y;
} cache[CACHE_SIZE];
struct cache *cache_free_head;


struct render_tile render_tiles[N_RENDER_TILES];
struct render_rect render_rects[N_RENDER_RECTS];
u32 n_render_tiles, n_render_rects;


static const u8 sizes[] = { 8, 16, 32, 64 };
static const u8 colour_sizes[] = { 2, 4, 6, 8 };


void render_kill_cache(void)
{
	u32 i;
	for (i = 0; i < CACHE_SIZE; i++) {
		cache[i].key = -1U;
		cache[i].next = 0;
	}

	for (i = HASH_SIZE; i < CACHE_SIZE - 1; i++)
		cache[i].next = &cache[i + 1];
	cache[CACHE_SIZE - 1].next = 0;
	cache_free_head = &cache[HASH_SIZE];
}

static u32 cache_key(u32 addr, u16 attr)
{
	return (attr << 24) | addr;
}

static u32 cache_hash(u32 key)
{
	return (key ^ (key >> 16)) % HASH_SIZE;
}

static struct cache *cache_get(u32 addr, u16 attr)
{
	u32 key = cache_key(addr, attr);

	struct cache *slot;
	for (slot = &cache[cache_hash(key)]; slot; slot = slot->next) {
		if (slot->key == 0)
			fatal("WHOOPS, CACHE WENT BAD (get)\n");
		if (slot->key == key)
			return slot;
	}

	fatal("WHOOPS, CACHE WENT BAD (get, slot)\n");
}

static struct cache *cache_make(u32 key)
{
	struct cache *slot = &cache[cache_hash(key)];
	if (slot->key == -1U)
		return slot;

	for ( ; slot->next; slot = slot->next) {
		if (slot->key == key)
			return slot;
		if (slot->key == -1U)
			fatal("WHOOPS, CACHE WENT BAD (make)\n");
	}

printf("make, new slot %04x\n", cache_free_head - cache);
	slot->next = cache_free_head;
	slot = cache_free_head;
	cache_free_head = cache_free_head->next;

	slot->next = 0;
	return slot;
}

void render_begin(void)
{
	n_render_tiles = 0;
	n_render_rects = 0;
	atlas_low_x = atlas_x;
	atlas_low_y = atlas_y;

	__render_begin();
}

static void render_texture_into_atlas(u32 addr, u16 attr, int w, int h)
{
	u32 key = cache_key(addr, attr);
	struct cache *cache = cache_make(key);

	if (cache->key == key)
		return;

	cache->key = key;

	if (atlas_x + w > ATLAS_W) {
		atlas_x = 0;
		atlas_y += atlas_h;
		atlas_h = 0;
	}

	if (atlas_y + h > ATLAS_H) {
		render_atlas(atlas_low_y, atlas_y + atlas_h - atlas_low_y);
		atlas_low_x = 0;
		atlas_low_y = 0;

		atlas_x = 0;
		atlas_y = 0;
		atlas_h = 0;

printf("filled atlas\n");

// XXX FIXME
		render_kill_cache();
	}

	u8 *p = &atlas[atlas_y*ATLAS_W + atlas_x];

//printf("rendering at %d %d\n", atlas_x, atlas_y);
	render_bitmap(p, ATLAS_W, addr, attr);

	cache->atlas_x = atlas_x;
	cache->atlas_y = atlas_y;

	atlas_x += w;

	if (h > atlas_h)
		atlas_h = h;
}

static void render_texture(u32 addr, u16 attr, int x, int y, int w, int h, u32 yline, u32 pal_offset, int use_alpha)
{
	struct cache *cache = cache_get(addr, attr);

	struct render_tile *tile = &render_tiles[n_render_tiles++];
	tile->x = x;
	tile->y = y;
	tile->w = w;
	tile->h = h;
	tile->atlas_x = cache->atlas_x;
	tile->atlas_y = cache->atlas_y + yline;
	tile->pal_offset = pal_offset;
	tile->alpha = (use_alpha ? mem[0x282a] << 6 : 255);
}

static void render_rect(int x, int y, int w, int h, u8 r, u8 g, u8 b)
{
	struct render_rect *rect = &render_rects[n_render_rects++];
	rect->x = x;
	rect->y = y;
	rect->w = w;
	rect->h = h;
	rect->r = r;
	rect->g = g;
	rect->b = b;
}

static void render_tile(u32 page, u32 xoff, u32 yoff, u16 attr, u16 ctrl, u16 tile)
{
	u32 h = sizes[(attr & 0x00c0) >> 6];
	u32 w = sizes[(attr & 0x0030) >> 4];

	u32 nc = colour_sizes[attr & 0x0003];

	u32 addr = 0x40*mem[0x2820 + page] + nc*w*h/16*tile;

	int use_alpha = !!(attr & 0x4000 || ctrl & 0x0100);
	int use_hoff = !!(ctrl & 0x0010);

	u32 palette_offset = (attr & 0x0f00) >> 4;
	palette_offset >>= nc;
	palette_offset <<= nc;

	if (use_hoff) {
		for (u32 y = 0; y < h; y++) {
			int yy = yoff + y;
			yy &= 0x01ff;
			if (yy >= 0x01c0)
				yy -= 0x0200;

			int xx = xoff;
			if (yy < 0 || yy >= 240)
				continue;

			xx -= mem[0x2900 + yy];

			xx &= 0x01ff;
			if (xx >= 0x01c0)
				xx -= 0x0200;

			if (xx + w <= 0 || xx >= 320)
				continue;

			render_texture(addr, attr, xx, yy, w, 1, y, palette_offset, use_alpha);
		}
	} else {
		int yy = yoff & 0x01ff;
		if (yy >= 0x01c0)
			yy -= 0x0200;

		int xx = xoff & 0x01ff;
		if (xx >= 0x01c0)
			xx -= 0x0200;

		if (xx + w <= 0 || xx >= 320)
			return;
		if (yy + h <= 0 || yy >= 240)
			return;

		render_texture(addr, attr, xx, yy, w, h, 0, palette_offset, use_alpha);
	}
}

static void render_tile_into_atlas(u32 page, u16 attr, u16 tile)
{
	u32 h = sizes[(attr & 0x00c0) >> 6];
	u32 w = sizes[(attr & 0x0030) >> 4];

	u32 nc = colour_sizes[attr & 0x0003];

	u32 addr = 0x40*mem[0x2820 + page] + nc*w*h/16*tile;

	render_texture_into_atlas(addr, attr, w, h);
}

static void __noinline render_page_into_atlas(u32 page)
{
	u16 *regs = &mem[0x2810 + 6*page];

	u32 attr = regs[2];
	u32 ctrl = regs[3];
	u32 tilemap = regs[4];
	u32 palette_map = regs[5];

	if ((ctrl & 8) == 0)
		return;

	u32 h = sizes[(attr & 0x00c0) >> 6];
	u32 w = sizes[(attr & 0x0030) >> 4];

	u32 hn = 256/h;
	u32 wn = 512/w;

	u32 x0, y0;
	for (y0 = 0; y0 < hn; y0++)
		for (x0 = 0; x0 < wn; x0++) {
			u32 which = (ctrl & 4) ? 0 : x0 + wn*y0;
			u16 tile = mem[tilemap + which];

			if (tile == 0)
				continue;

			u16 palette = mem[palette_map + which/2];
			if (which & 1)
				palette >>= 8;

			u32 tileattr = attr;
			if ((ctrl & 2) == 0) {
				tileattr &= ~0x000c;
				tileattr |= (palette >> 2) & 0x000c;	// flip
			}

			render_tile_into_atlas(page, tileattr, tile);
		}
}

static void render_sprite_into_atlas(u16 *sprite)
{
	u16 tile, attr;

	tile = sprite[0];
	attr = sprite[3];

	if (tile == 0)
		return;

	render_tile_into_atlas(2, attr, tile);
}

static void __noinline render_sprites_into_atlas(void)
{
	if ((mem[0x2842] & 1) == 0)
		return;

	u32 i;
	for (i = 0; i < 256; i++)
		render_sprite_into_atlas(mem + 0x2c00 + 4*i);
}

static void __noinline render_page(u32 page, u32 depth)
{
	u16 *regs = &mem[0x2810 + 6*page];

	u32 xscroll = regs[0];
	u32 yscroll = regs[1];
	u32 attr = regs[2];
	u32 ctrl = regs[3];
	u32 tilemap = regs[4];
	u32 palette_map = regs[5];

	if ((ctrl & 8) == 0)
		return;

	if ((attr & 0x3000) >> 12 != depth)
		return;

	u32 h = sizes[(attr & 0x00c0) >> 6];
	u32 w = sizes[(attr & 0x0030) >> 4];

	u32 hn = 256/h;
	u32 wn = 512/w;

	u32 x0, y0;
	for (y0 = 0; y0 < hn; y0++)
		for (x0 = 0; x0 < wn; x0++) {
			u32 which = (ctrl & 4) ? 0 : x0 + wn*y0;
			u16 tile = mem[tilemap + which];

			if (tile == 0)
				continue;

			u16 palette = mem[palette_map + which/2];
			if (which & 1)
				palette >>= 8;

			u32 tileattr = attr;
			u32 tilectrl = ctrl;
			if ((ctrl & 2) == 0) {
// -(1) bld(1) flip(2) pal(4)
				tileattr &= ~0x000c;
				tileattr |= (palette >> 2) & 0x000c;	// flip

				tileattr &= ~0x0f00;
				tileattr |= (palette << 8) & 0x0f00;	// palette

				tilectrl &= ~0x0100;
				tilectrl |= (palette << 2) & 0x0100;	// blend
			}

			u32 yy = ((h*y0 - yscroll + 0x10) & 0xff) - 0x10;
			u32 xx = (w*x0 - xscroll) & 0x1ff;

			render_tile(page, xx, yy, tileattr, tilectrl, tile);
		}
}

static void render_sprite(u32 depth, u16 *sprite)
{
	u16 tile, attr;
	s16 x, y;

	tile = sprite[0];
	x = sprite[1];
	y = sprite[2];
	attr = sprite[3];

	if (tile == 0)
		return;

	if ((u32)(attr & 0x3000) >> 12 != depth)
		return;

	if (board->use_centered_coors) {
		x = 160 + x;
		y = 120 - y;

		u32 h = sizes[(attr & 0x00c0) >> 6];
		u32 w = sizes[(attr & 0x0030) >> 4];

		x -= (w/2);
		y -= (h/2) - 8;
	}

	x = x & 0x1ff;
	y = y & 0x1ff;

	render_tile(2, x, y, attr, 0, tile);
}

static void __noinline render_sprites(u32 depth)
{
	if ((mem[0x2842] & 1) == 0)
		return;

	u32 i;
	for (i = 0; i < 256; i++)
		render_sprite(depth, mem + 0x2c00 + 4*i);
}

static void do_show_fps(void)
{
	static u32 fps = 0;

	const int per_n = 5;
	static int count = 0;
	count++;
	if (count == per_n) {
		static u32 last = 0;
		u32 now = get_realtime();
		if (last)
			fps = 1000000 * per_n / (now - last);
		last = now;
		count = 0;
	}

	if (fps > 50)
		fps = 50;

	render_rect(3, 3, 7, 52, 255, 255, 255);
	render_rect(4, 4, 5, 50 - fps, 0, 0, 0);
	render_rect(4, 54 - fps, 5, fps, 0, 255, 0);
}

void render(void)
{
	render_begin();

	if ((mem[0x3d20] & 0x0004) == 0) {	// video DAC disable
		render_palette();

		if (!hide_page_1)
			render_page_into_atlas(0);
		if (!hide_page_2)
			render_page_into_atlas(1);
		if (!hide_sprites)
			render_sprites_into_atlas();

		u32 depth;
		for (depth = 0; depth < 4; depth++) {
			if (!hide_page_1)
				render_page(0, depth);
			if (!hide_page_2)
				render_page(1, depth);
			if (!hide_sprites)
				render_sprites(depth);
		}
	}

	if (atlas_y != atlas_low_y || atlas_x != atlas_low_x)
		render_atlas(atlas_low_y, atlas_y + atlas_h - atlas_low_y);

	if (show_fps)
		do_show_fps();

	render_end();

	update_screen();
}
