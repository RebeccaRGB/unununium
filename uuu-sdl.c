// Copyright 2008  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include <stdio.h>
#include <string.h>
#include <time.h>

// We need to include this here, since it redefines main().
#include <SDL.h>

#include "types.h"
#include "platform.h"
#include "emu.h"
#include "dialog.h"


#ifdef __APPLE__
void CGSSetConnectionProperty(int, int, int, int);
int _CGSDefaultConnection();
int CGSCreateCString(char *);
int CGSCreateBoolean(unsigned char);
void CGSReleaseObj(int);
#endif


int main(int argc, char *argv[])
{
	FILE *in;

#ifdef __APPLE__
	if (argc == 1) {
		argv[1] = (char *)dialog_rom_file();
		if (argv[1])
			argc = 2;
	}
#endif

	if (argc != 2)
		fatal("usage: %s <rom-file>\n", argv[0]);

	in = fopen(argv[1], "rb");
	if (!in)
		fatal("Cannot read ROM file %s\n", argv[1]);
	rom_file = in;

#ifdef __APPLE__
	// Hack to speed up display refresh
	int propertyString = CGSCreateCString("DisableDeferredUpdates");
	int cid = _CGSDefaultConnection();
	CGSSetConnectionProperty(cid, cid, propertyString, CGSCreateBoolean(1));
	CGSReleaseObj(propertyString);
#endif

	srandom(time(0));

	emu();

	return 0;
}
