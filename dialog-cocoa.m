// Copyright 2008-2010  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#import <Cocoa/Cocoa.h>

#include "dialog.h"

static char path[256];

const char *dialog_rom_file(void)
{
	NSOpenPanel *panel = [NSOpenPanel openPanel];

	NSString *dir = [[NSBundle mainBundle] resourcePath];
	dir = [dir stringByAppendingPathComponent:@"ROMs"];

	NSArray *types = nil;//[NSArray arrayWithObject:@"uuu"];
	//[panel setAllowsOtherFileTypes:YES];

	int result = [panel runModalForDirectory:dir file:nil types:types];

	if (result != NSOKButton)
		return 0;

	[[panel filename] getCString:path maxLength:(sizeof path)];

	return path;
}
