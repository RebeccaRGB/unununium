// Copyright 2008,2009  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#ifndef _TYPES_H
#define _TYPES_H

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef signed short s16;
typedef signed int s32;

#define ARRAY_SIZE(x) (sizeof x / sizeof x[0])

#if 0
#define subtype(_type, _base, _field) \
	(void *)((u8 *)(_base) - __builtin_offsetof(_type, _field))
#else
#define subtype(_type, _base, _field) \
       (void *)((u8 *)(_base) - ((u8 *)&(((_type *)0)->_field) - 0))
#endif

#endif
