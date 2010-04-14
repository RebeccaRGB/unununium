// Copyright 2010  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include <stdio.h>
#include <string.h>
#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#else
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#endif

#include "types.h"
#include "emu.h"
#include "platform.h"

#include "render.h"


static GLuint atlas_texture, palette_texture, page_coor_texture,
              page_colour_texture, page_hoff_texture;
static GLuint tile_fragment_program, page_fragment_program;
static GLuint display_list;


#define error() do { int err = glGetError(); if (err) fatal("GL error 0x%04x at line %d\n", err, __LINE__); } while(0)


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
	glBindTexture(GL_TEXTURE_1D, palette_texture);
	glTexSubImage1D(GL_TEXTURE_1D, 0, 0, 256, GL_BGRA, GL_UNSIGNED_BYTE, that);
error();
}


static enum render_state {
	state_nothing,
	state_page,
	state_tile,
	state_rect
} current_render_state;

static void set_render_state(enum render_state render_state)
{
	if (render_state == current_render_state)
		return;
	current_render_state = render_state;

	switch (render_state) {
	case state_nothing:
		break;

	case state_page:
		glEnable(GL_FRAGMENT_PROGRAM_ARB);
		glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, page_fragment_program);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, atlas_texture);
		glEnable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE2);
		glEnable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE3);
		glEnable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE4);
		glEnable(GL_TEXTURE_1D);
		break;

	case state_tile:
		glEnable(GL_FRAGMENT_PROGRAM_ARB);
		glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, tile_fragment_program);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, atlas_texture);
		glEnable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE2);
		glDisable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE3);
		glDisable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE4);
		glDisable(GL_TEXTURE_1D);
		break;

	case state_rect:
		glDisable(GL_FRAGMENT_PROGRAM_ARB);
		glActiveTexture(GL_TEXTURE0);
		glDisable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE2);
		glDisable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE3);
		glDisable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE4);
		glDisable(GL_TEXTURE_1D);
	}
}


void render_page(u32 xscroll, u32 yscroll, u32 h, u32 w)
{
	set_render_state(state_page);

	u16 coors[4096];
	u8 colours[4096];
	u16 hoff[256];

	u32 wn = 512/w, hn = 256/h;
	u32 x, y;

	u16 *coors1 = render_page_atlas_coors;
	u8 *colours1 = render_page_tile_colours;

	for (y = 0; y < hn; y++) {
		for (x = 0; x < wn; x++) {
			coors[128*y + 2*x] = 32 * *coors1++;
			coors[128*y + 2*x + 1] = 32 * *coors1++;
			colours[128*y + 2*x] = *colours1++;
			colours[128*y + 2*x + 1] = *colours1++;
		}
		for ( ; x < 64; x += wn) {
			memcpy(&coors[128*y + 2*x], &coors[128*y], 4*wn);
			memcpy(&colours[128*y + 2*x], &colours[128*y], 2*wn);
		}
	}
	memcpy(&coors[128*hn], coors, 256*(32-hn));
	memcpy(&colours[128*hn], colours, 128*(32-hn));

	glActiveTexture(GL_TEXTURE2);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, page_coor_texture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 64, 32, GL_LUMINANCE_ALPHA,
	                GL_UNSIGNED_SHORT, coors);

	glActiveTexture(GL_TEXTURE3);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, page_colour_texture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 64, 32, GL_LUMINANCE_ALPHA,
	                GL_UNSIGNED_BYTE, colours);

	for (y = 0; y < 256; y++)
		hoff[y] = 128*render_page_hoff[y];

	glActiveTexture(GL_TEXTURE4);
	glEnable(GL_TEXTURE_1D);
	glBindTexture(GL_TEXTURE_1D, page_hoff_texture);
	glTexSubImage1D(GL_TEXTURE_1D, 0, 0, 256, GL_LUMINANCE,
	                GL_UNSIGNED_SHORT, hoff);

	glProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 0,
	                             (float)xscroll/w, (float)yscroll/h, 0, 0);
	glProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 1,
	                             512.0f/w, 256.0f/h, 0, 0);
	glProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 2,
	                             w/2048.0f, h/2048.0f, 0, 0);

	float tx0 = 0.0f;
	float ty0 = 0.0f;
	float tx1 = 320.0f/512;
	float ty1 = 240.0f/256;

	// X and Y texture coordinates swapped to avoid a swizzle on
	// the first TEX insn (the hoff one), since that costs an extra
	// indirection on some cards, and then we have too many.
	glBegin(GL_QUADS);
	glTexCoord2f(ty0, tx0);
	glVertex2i(0, 0);
	glTexCoord2f(ty0, tx1);
	glVertex2i(320, 0);
	glTexCoord2f(ty1, tx1);
	glVertex2i(320, 240);
	glTexCoord2f(ty1, tx0);
	glVertex2i(0, 240);
	glEnd();
error();
}

void render_texture(int x, int y, int w, int h, u16 atlas_x, u16 atlas_y, u8 pal_offset, u8 alpha)
{
	set_render_state(state_tile);

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

void render_rect(int x, int y, int w, int h, u8 r, u8 g, u8 b)
{
	set_render_state(state_rect);

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
debug("atlas: %d+%d\n", atlas_y, atlas_h);

	glBindTexture(GL_TEXTURE_2D, atlas_texture);
error();
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, atlas_y, ATLAS_W, atlas_h, GL_LUMINANCE, GL_UNSIGNED_BYTE, &atlas[atlas_y*ATLAS_W]);
error();
}


void render_begin(void)
{
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);

	set_render_state(state_nothing);
}

void render_end(void)
{
}


static void init_textures(void)
{
	glGenTextures(1, &palette_texture);
	glBindTexture(GL_TEXTURE_1D, palette_texture);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage1D(GL_TEXTURE_1D, 0, 4, 256, 0, GL_BGRA, GL_UNSIGNED_BYTE, 0);

	glGenTextures(1, &atlas_texture);
	glBindTexture(GL_TEXTURE_2D, atlas_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, 1, ATLAS_W, ATLAS_H, 0, GL_LUMINANCE,
	             GL_UNSIGNED_BYTE, 0);

	glGenTextures(1, &page_coor_texture);
	glBindTexture(GL_TEXTURE_2D, page_coor_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE16_ALPHA16, 64, 32, 0,
	             GL_LUMINANCE_ALPHA, GL_UNSIGNED_SHORT, 0);

	glGenTextures(1, &page_colour_texture);
	glBindTexture(GL_TEXTURE_2D, page_colour_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, 2, 64, 32, 0, GL_LUMINANCE_ALPHA,
	             GL_UNSIGNED_BYTE, 0);

	glGenTextures(1, &page_hoff_texture);
	glBindTexture(GL_TEXTURE_1D, page_hoff_texture);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage1D(GL_TEXTURE_1D, 0, GL_LUMINANCE16, 256, 0,
	             GL_LUMINANCE, GL_UNSIGNED_SHORT, 0);

error();
}

static void init_fragment_program(GLuint *id, const char *program)
{
	glGenProgramsARB(1, id);
	glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, *id);

	glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
	                   strlen(program), program);
	GLint err;
	glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &err);
	if (err != -1) {
		printf("problem compiling fragment program:\n");
		printf("  err pos = %ld\n", (long)err);
		printf("  err msg: %s\n",
		       glGetString(GL_PROGRAM_ERROR_STRING_ARB));
		fatal("");
	}

	GLint n, nn;
	glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB,
	                  GL_PROGRAM_INSTRUCTIONS_ARB, &n);
	glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB,
	                  GL_PROGRAM_NATIVE_INSTRUCTIONS_ARB, &nn);
	debug(">>> %d (%d) insns\n", n, nn);

	glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB,
	                  GL_PROGRAM_TEMPORARIES_ARB, &n);
	glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB,
	                  GL_PROGRAM_NATIVE_TEMPORARIES_ARB, &nn);
	debug(">>> %d (%d) temps\n", n, nn);

	glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB,
	                  GL_PROGRAM_PARAMETERS_ARB, &n);
	glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB,
	                  GL_PROGRAM_NATIVE_PARAMETERS_ARB, &nn);
	debug(">>> %d (%d) params\n", n, nn);

	glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB,
	                  GL_PROGRAM_ATTRIBS_ARB, &n);
	glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB,
	                  GL_PROGRAM_NATIVE_ATTRIBS_ARB, &nn);
	debug(">>> %d (%d) attrs\n", n, nn);

	glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB,
	                  GL_PROGRAM_ALU_INSTRUCTIONS_ARB, &n);
	glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB,
	                  GL_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB, &nn);
	debug(">>> %d (%d) alu insns\n", n, nn);

	glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB,
	                  GL_PROGRAM_TEX_INSTRUCTIONS_ARB, &n);
	glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB,
	                  GL_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB, &nn);
	debug(">>> %d (%d) tex insns\n", n, nn);

	glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB,
	                  GL_PROGRAM_TEX_INDIRECTIONS_ARB, &n);
	glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB,
	                  GL_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB, &nn);
	debug(">>> %d (%d) tex indirs\n", n, nn);

	GLint ok;
	glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB,
	                  GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB, &ok);
	if (!ok)
		fatal("fragment program exceeds resources\n");
}

static void init_fragment_programs(void)
{
	const char tile_frag[] =
		"!!ARBfp1.0\n"
		"OPTION ARB_precision_hint_nicest; \n"

		"TEMP r0; \n"

		"TEX r0.x, fragment.texcoord[0], texture[0], 2D; \n"	//**
		"ADD r0.x, r0.x, fragment.color.r; \n"

		"TEX r0, r0, texture[1], 1D; \n"			//**
		"MUL r0.a, r0.a, fragment.color.a; \n"
		"MOV result.color, r0; \n"

		"END";
	init_fragment_program(&tile_fragment_program, tile_frag);

	const char page_frag[] =
		"!!ARBfp1.0\n"
		"OPTION ARB_precision_hint_nicest; \n"

		"TEMP coor, tile_coor, tile, tile_colour; \n"
		"TEMP atlas_coor, palette_coor, colour, hoff; \n"

		"TEX hoff, fragment.texcoord[0], texture[4], 1D; \n"	//**

		"MOV coor, fragment.texcoord[0]; \n"
		"ADD coor.y, coor, hoff; \n"
		"MAD coor, coor.yxxx, program.local[1], program.local[0]; \n"
		// now coor is integer tile #, fraction coor within tile

		"FLR tile_coor, coor; \n"
		"FRC atlas_coor, coor; \n"

		// 1/64,1/32
		"PARAM tile_scale = { 0.015625, 0.03125 }; \n"
		"MUL tile_coor, tile_coor, tile_scale; \n"

		"TEX tile, tile_coor, texture[2], 2D; \n"		//**
		"TEX tile_colour, tile_coor, texture[3], 2D; \n"
		"MAD atlas_coor, atlas_coor, program.local[2], tile.xwww; \n"

		"TEX palette_coor, atlas_coor, texture[0], 2D; \n"	//**
		"ADD palette_coor, palette_coor, tile_colour.r; \n"

		"TEX colour, palette_coor, texture[1], 1D; \n"		//**
		"MUL colour.a, colour.a, tile_colour.a; \n"
		"MOV result.color, colour; \n"

		"END";
	init_fragment_program(&page_fragment_program, page_frag);
}

void render_init(u32 pixel_size)
{
	debug("GL RENDERER: %s\n", glGetString(GL_RENDERER));
	debug("GL VENDOR: %s\n", glGetString(GL_VENDOR));
//	debug("GL EXTENSIONS: %s\n", glGetString(GL_EXTENSIONS));
	debug("GL VERSION: %s\n", glGetString(GL_VERSION));


	glViewport(0, 0, pixel_size*320, pixel_size*240);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 320, 240, 0, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	error();

	init_textures();
	init_fragment_programs();

	display_list = glGenLists(1);
}
