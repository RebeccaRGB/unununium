// Copyright 2008,2009  Segher Boessenkool  <segher@kernel.crashing.org>
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
#ifdef __APPLE__
	if (argc == 1) {
		const char *path = dialog_rom_file();
		if (!path)
			return 0;
		open_rom(path);
	}
#else
	if (0)
		;
#endif
	else {
		if (argc != 2)
			fatal("usage: %s <rom-file>\n", argv[0]);

		open_rom(argv[1]);
	}

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
