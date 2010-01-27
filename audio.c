// Copyright 2008,2009  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include <stdio.h>
	#include <math.h>

#include "types.h"
#include "emu.h"

#include "audio.h"


int mute_audio = 1;


void audio_store(u16 val, u32 addr)
{
	if (addr == 0x340b)
		mem[addr] &= ~val;
	else
		mem[addr] = val;

	if (addr < 0x3200) {		// XXX
		return;
	} else if (addr < 0x3400) {	// XXX
		return;
	} else {			// XXX
//		if (addr == 0x3400)
//			printf("CHANNELS: %04x\n", val);
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
//		if ((addr & 0x0f) == 0x0f)	// channel status
//			return mem[addr - 0x0f];
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

static void set_channel_bit(u32 ch, u32 reg, u32 val)
{
	reg += 0x3400;
	if (ch >= 16) {
		reg += 0x0100;
		ch -= 16;
	}
	u16 mask = 1 << ch;
	mem[reg] = (mem[reg] & ~mask) | (val << ch);
}

static const s32 adpcm_index[] = {
	-1, -1, -1, -1, 2, 4, 6, 8,
	-1, -1, -1, -1, 2, 4, 6, 8
};
static const u16 adpcm_step[] = {
	7, 8, 9, 10, 11, 12, 13, 14,
	16, 17, 19, 21, 23, 25, 28, 31,
	34, 37, 41, 45, 50, 55, 60, 66,
	73, 80, 88, 97, 107, 118, 130, 143,
	157, 173, 190, 209, 230, 253, 279, 307,
	337, 371, 408, 449, 494, 544, 598, 658,
	724, 796, 876, 963, 1060, 1166, 1282, 1411,
	1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024,
	3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484,
	7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
	15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
	32767
};
static u32 adpcm_si[24];
static void do_adpcm(u32 ch, u32 nybble)
{
	s32 si = adpcm_si[ch];
	u32 step = adpcm_step[si];

	si += adpcm_index[nybble];
	if (si < 0)
		si = 0;
	if (si > 88)
		si = 88;
	adpcm_si[ch] = si;

	s32 what = mem[0x300b + 16*ch];
	what += (2 * (nybble & 7) + 1) * step / 8;
	if (nybble & 8)
		what -= step;
	if (what < 0)
		what = 0;
	if (what > 0xffff)
		what = 0xffff;
	mem[0x300b + 16*ch] = what;
}

static u32 n_left[24];
static u16 bits_left[24];
static void do_next_sample(u32 ch)
{
loop:
	if (n_left[ch] == 0) {
		u32 addr = ((mem[0x3001 + 16*ch] & 0x3f) << 16) | mem[0x3000 + 16*ch];
		if (addr == 0) {
			printf("uh-oh, looping where i shouldn't: mode=%04x\n", mem[0x3001 + 16*ch] & 0xf000);
			mem[0x300b + 16*ch] = 0x8000;
			return;
		}
		bits_left[ch] = mem[addr];
		if (bits_left[ch] == 0xffff) {
			switch (mem[0x3001 + 16*ch] >> 14) {
			case 0:
				mem[0x3000 + 16*ch] = mem[0x3002 + 16*ch];
				mem[0x3001 + 16*ch] &= 0xffc0;
				mem[0x3001 + 16*ch] |= (mem[0x3001 + 16*ch] >> 6) & 0x3f;
				goto loop;

			case 3:
case 1: case 2:
				set_channel_bit(ch, 0xd, 1);
				mem[0x300b + 16*ch] = 0x8000;
				return;

			default:
				printf("sound: unhandled mode: %04x\n", mem[0x3001 + 16*ch] & 0xf000);
				set_channel_bit(ch, 0xd, 1);
				mem[0x300b + 16*ch] = 0x8000;
				return;
			}
		}
		n_left[ch] = 2;
		mem[0x3000 + 16*ch]++;
		if (mem[0x3000 + 16*ch] == 0)
			mem[0x3001 + 16*ch]++;	// XXX: mask?
//printf("--> addr = %06x\n", addr);
	}

	switch ((mem[0x3001 + 16*ch] & 0x3000) >> 12) {
	case 1:		// ADPCM
//		do_adpcm(ch, bits_left[ch] & 0xf);
//		bits_left[ch] >>= 4;
//		n_left[ch]--;
		break;

	case 2:		// U8
		mem[0x300b + 16*ch] = bits_left[ch] << 8;
		bits_left[ch] >>= 8;
		n_left[ch]--;
		break;

	default:
		printf("sound: unhandled mode: %04x\n", mem[0x3001 + 16*ch] & 0xf000);
		mem[0x300b + 16*ch] = 0x8000;
	}
}

#define FREQ 440
static u16 next_channel_sample(u32 ch)
{
//	if (ch != 1)
//		return 0x8000;

//	static u32 xxx[24] = {0};
//	u16 sample = 0x8000 + (int)(20000*sin(2*M_PI*xxx[ch]*FREQ/281250));
//	xxx[ch]++;
//	if (xxx[ch] == 281250)
//		xxx[ch] = 0;
//	return sample;

	u16 sample = mem[0x300b + 16*ch];
	u32 acc = (mem[0x3201 + 16*ch] << 16) | mem[0x3205 + 16*ch];
	u32 inc = (mem[0x3200 + 16*ch] << 16) | mem[0x3204 + 16*ch];
	acc += inc;
	if (acc >= 0x80000) {
		do_next_sample(ch);
		acc -= 0x80000;
	}
	mem[0x3201 + 16*ch] = acc >> 16;
	mem[0x3205 + 16*ch] = acc;

	return sample;
}

static u16 next_sample(void)
{
	static u32 ch = 0;
	u16 sample;
	if (get_channel_bit(ch, 0) && !get_channel_bit(ch, 0xb))
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
return 0;
	static s16 old[153];
	static double coss[153];
	static int kk = 0;
	if (kk == 0) {
		kk = 1;
		for (int j = 0; j < 153; j++)
			coss[j] = 0.5 - 0.5*cos(M_PI*j/153);
	}
	double acc = 0;
	u32 i;
	for (i = 0; i < 153; i++)
		acc += coss[i] * old[i];
	for (i = 0; i < 153; i++)
		old[i] = next_sample() - 0x8000;
	for (i = 0; i < 153; i++)
		acc += (1.0 - coss[i]) * old[i];
	int clip = acc / 153;
	if (clip > 32767)
		clip = 32767;
	if (clip < -32769)
		clip = -32768;
	return clip;
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
