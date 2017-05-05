// Wireless 60 board by Rebecca Bettencourt
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include <stdlib.h>
#include <stdio.h>

#include "types.h"
#include "emu.h"
#include "platform.h"

#include "board.h"

static u32 current_bank = -1;

static void switch_bank(u32 bank) {
	if (bank == current_bank) return;
	current_bank = bank;
	render_kill_cache();
	read_rom(N_MEM * bank);
	// printf("bank: %d\n", bank);

	board->idle_pc = 0;
	if (bank) {
		if (mem[0x3ff1c] == 0x4311 && mem[0x3ff1e] == 0x4e43)	// Wireless 60
			board->idle_pc = 0x3ff1c;
		if (mem[0x7e5a8] == 0x4311 && mem[0x7e5aa] == 0x4e43)	// Zone 60
			board->idle_pc = 0x7e5a8;
	}
}

static void init(void) {
	switch_bank(0);
}

static int controller_input = 0;

static u16 gpio(u32 n, u16 what, __unused u16 push, __unused u16 pull, __unused u16 special) {
	// printf("-->  port %c  what %04x  push %04x  pull %04x\n", 'A'+n, what, push, pull);
	// printf("\rbank: %d", current_bank);
	switch (n) {
		case 1:
			switch_bank(what & 7);
			break;
		case 0:
			switch (what & 0x300) {
				case 0x300:
					controller_input = 0;
					break;
				case 0x200:
					controller_input++;
					break;
				default:
					switch (controller_input) {
						case 1: if (button_A)     what ^= 0x400; break; /* what ^= 0x800; for player 2 */
						case 2: if (button_B)     what ^= 0x400; break;
						case 3: if (button_menu)  what ^= 0x400; break;
						case 4: if (button_C)     what ^= 0x400; break; /* button C = start */
						case 5: if (button_up)    what ^= 0x400; break;
						case 6: if (button_down)  what ^= 0x400; break;
						case 7: if (button_left)  what ^= 0x400; break;
						case 8: if (button_right) what ^= 0x400; break;
					}
					break;
			}
			break;
	}
	return what;
}

struct board board_W60 = {
	.use_centered_coors = 1,
	.init = init,
	.gpio = gpio
};
