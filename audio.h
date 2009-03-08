// Copyright 2008,2009  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#ifndef _AUDIO_H
#define _AUDIO_H

#include "types.h"


extern int mute_audio;


void audio_store(u16 val, u32 addr);
u16 audio_load(u32 addr);

void audio_render(s16 *data, u32 n);

void audio_init(void);

#endif
