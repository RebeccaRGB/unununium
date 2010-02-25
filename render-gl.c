// Copyright 2010  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include <stdio.h>
#include <string.h>
#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#else
#include <GL/gl.h>
#include <GL/glext.h>
#endif

#include "types.h"
#include "emu.h"
#include "platform.h"

#include "render.h"


static GLuint palette_texture;
static GLuint atlas_texture;
static GLuint fragment_shader;
static GLuint display_list;


#define error() do { int err = glGetError(); if (err) fatal("GL error 0x%04x at line %d\n", err, __LINE__); } while(0)
//#define error() do { } while (0)


void render_palette(void)
{
	static u8 that[4*256];

	u32 i;
	for (i = 0; i < 256; i++) {
		u16 p = mem[0x2b00 + i];
		that[4*i] = 255*(p & 31) / 31.0f;
		that[4*i+1] = 255*((p >> 5) & 31) / 31.0f;
		that[4*i+2] = 255*((p >> 10) & 31) / 31.0f;
		that[4*i+3] = 255*(1-(p >> 15));
	}

	glActiveTexture(GL_TEXTURE1);
error();
	glBindTexture(GL_TEXTURE_1D, palette_texture);
error();
	glTexSubImage1D(GL_TEXTURE_1D, 0, 0, 256, GL_BGRA, GL_UNSIGNED_BYTE, that);
error();
}


void __render_begin(void)
{
}

static void __noinline do_render_texture(int x, int y, int w, int h, u32 atlas_x, u32 atlas_y, u8 pal_offset, u8 alpha)
{
	glColor4ub(pal_offset, 0, 0, alpha);
error();

	float tx0 = (float)atlas_x/ATLAS_W;
	float ty0 = (float)atlas_y/ATLAS_H;
	float tx1 = (float)(atlas_x + w)/ATLAS_W;
	float ty1 = (float)(atlas_y + h)/ATLAS_H;

	glBegin(GL_QUADS);
	glTexCoord2f(tx0, ty0);
	glVertex2i(x, y);
	glTexCoord2f(tx1, ty0);
	glVertex2i(x + w, y);
	glTexCoord2f(tx1, ty1);
	glVertex2i(x + w, y + h);
	glTexCoord2f(tx0, ty1);
	glVertex2i(x, y + h);
	glEnd();
error();
}

static void do_render_rect(int x, int y, int w, int h, u8 r, u8 g, u8 b)
{
	glBegin(GL_QUADS);
	glColor3ub(r, g, b);
	glVertex2i(x, y);
	glVertex2i(x + w, y);
	glVertex2i(x + w, y + h);
	glVertex2i(x, y + h);
	glEnd();
error();
}

void render_atlas(u32 atlas_y, u32 atlas_h)
{
printf("atlas: %d+%d\n", atlas_y, atlas_h);

	glBindTexture(GL_TEXTURE_2D, atlas_texture);
error();
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, atlas_y, ATLAS_W, atlas_h, GL_LUMINANCE, GL_UNSIGNED_BYTE, &atlas[atlas_y*ATLAS_W]);
error();
}

void render_end(void)
{
	u32 i;

	glClearColor(0.25, 0.15, 0.10, 0);
error();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
error();

	glNewList(display_list, GL_COMPILE_AND_EXECUTE);
error();

	glEnable(GL_FRAGMENT_PROGRAM_ARB);
error();
	glActiveTexture(GL_TEXTURE0);
error();
	glEnable(GL_TEXTURE_2D);
error();


	for (i = 0; i < n_render_tiles; i++) {
		struct render_tile *tile = &render_tiles[i];
		do_render_texture(tile->x, tile->y, tile->w, tile->h, tile->atlas_x, tile->atlas_y, tile->pal_offset, tile->alpha);
	}

	glActiveTexture(GL_TEXTURE0);
error();
	glDisable(GL_TEXTURE_2D);
error();
	glDisable(GL_FRAGMENT_PROGRAM_ARB);
error();

	for (i = 0; i < n_render_rects; i++) {
		struct render_rect *rect = &render_rects[i];
		do_render_rect(rect->x, rect->y, rect->w, rect->h, rect->r, rect->g, rect->b);
	}

	glEndList();
error();
}


static void init_palette(void)
{
	glGenTextures(1, &palette_texture);
error();
	glBindTexture(GL_TEXTURE_1D, palette_texture);
error();
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
error();
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
error();
	glTexImage1D(GL_TEXTURE_1D, 0, 4, 256, 0, GL_BGRA, GL_UNSIGNED_BYTE, 0);
error();
}

static void init_textures(void)
{
	glGenTextures(1, &atlas_texture);
error();

	glBindTexture(GL_TEXTURE_2D, atlas_texture);
error();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
error();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
error();
	glTexImage2D(GL_TEXTURE_2D, 0, 1, ATLAS_W, ATLAS_H, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, 0);
error();
}

static void init_fragment_shader(void)
{
	glGenProgramsARB(1, &fragment_shader);
error();
	glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, fragment_shader);
error();
	const char frag[] =
		"!!ARBfp1.0\n"
		"TEMP r0; \n"
		"TEX r0.x, fragment.texcoord[0], texture[0], 2D; \n"
		"ADD r0.x, r0.x, fragment.color.r; \n"
		"TEX r0, r0, texture[1], 1D; \n"
		"MUL r0.a, r0.a, fragment.color.a; \n"
		"MOV result.color, r0; \n"
		"END";
	glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, strlen(frag), frag);
GLint err; glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &err);
printf("GL said err pos = %ld\n", (long)err);	// (-1 if ok)
printf("GL has this to say: >>>%s<<<\n", glGetString(GL_PROGRAM_ERROR_STRING_ARB));
error();
	glEnable(GL_FRAGMENT_PROGRAM_ARB);
error();
}

void render_init(u32 pixel_size)
{
	printf("GL RENDERER: %s\n", glGetString(GL_RENDERER));
	printf("GL VENDOR: %s\n", glGetString(GL_VENDOR));
	printf("GL EXTENSIONS: %s\n", glGetString(GL_EXTENSIONS));
	printf("GL VERSION: %s\n", glGetString(GL_VERSION));


	glViewport(0, 0, pixel_size*320, pixel_size*240);
error();
	glMatrixMode(GL_PROJECTION);
error();
	glLoadIdentity();
error();
	glOrtho(0, 320, 240, 0, -1, 1);
error();
	glMatrixMode(GL_MODELVIEW);
error();
	glLoadIdentity();
error();

	glEnable (GL_BLEND);
error();
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
error();

	init_palette();
	init_textures();
	init_fragment_shader();

	display_list = glGenLists(1);
error();
}
