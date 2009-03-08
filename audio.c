// Copyright 2008,2009  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include <stdio.h>
	#include <math.h>

#include "types.h"
#include "emu.h"

#include "audio.h"


int mute_audio;


void audio_store(u16 val, u32 addr)
{
	mem[addr] = val;

	if (addr < 0x3200) {		// XXX
		return;
	} else if (addr < 0x3400) {	// XXX
		return;
	} else {			// XXX
		return;
	}

	printf("AUDIO STORE %04x to %04x\n", val, addr);
}

u16 audio_load(u32 addr)
{
	u16 val = mem[addr];

	if (addr >= 0x3000 && addr < 0x3200) {		// audio something
		//printf("LOAD %04x from %04x\n", val, addr);
		return val;
	}
	if (addr >= 0x3200 && addr < 0x3300) {		// audio something
		//printf("LOAD %04x from %04x\n", val, addr);
		return val;
	}
	if (addr >= 0x3400 && addr < 0x3500) {		// audio something
		//printf("LOAD %04x from %04x\n", val, addr);
		return val;
	}

	printf("UNKNOWN AUDIO LOAD from %04x\n", addr);
	return val;
}

static u32 get_channel_bit(u32 ch, u32 reg)
{
	reg += 0x3400;
	if (ch >= 16) {
		reg += 0x0100;
		ch -= 16;
	}
	return (mem[reg] >> ch) & 1;
}

#define FREQ 440
static u16 next_channel_sample(u32 ch)
{
//	if (ch)
//		return 0x8000;
	static u32 xxx[24] = {0};
	u16 sample = 0x8000 + (int)(20000*sin(2*M_PI*xxx[ch]*FREQ/281250));
	xxx[ch]++;
	if (xxx[ch] == 281250)
		xxx[ch] = 0;
	return sample;
}

static u16 next_sample(void)
{
	static u32 ch = 0;
	u16 sample;
	if (get_channel_bit(ch, 0))
		sample = next_channel_sample(ch);
	else
		sample = 0x8000;
	ch++;
	if (ch == 24)
		ch = 0;
	return sample;
}

static s16 next_host_sample(void)
{
	u32 acc = 0;
	u32 i;
	for (i = 0; i < 153; i++)
		acc += next_sample();
	return (acc / 153) - 0x8000;
}

void audio_render(s16 *data, u32 n)
{
	u32 i;
	for (i = 0; i < n; i++)
		data[i] = next_host_sample();
}

void audio_init(void)
{
}
