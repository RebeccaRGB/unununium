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

	[[panel filename] getCString:path maxLength:(sizeof path)
	                                  encoding:NSASCIIStringEncoding];

	return path;
}
