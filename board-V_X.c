// Copyright 2010  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt


// IOA not connected

// IOB0   CS#1 = a3; b3 = ROMCS#; o3 = cart CS1#
// IOB1   CS#2 = cart CS2#
// IOB2   CS#3 = a0; b0 = cart det (high if cart, pull to ROMCS#); o0 = sys CS#
// IOB3   reset switch
// IOB4   cart front 24?
// IOB5   power off
// IOB6   power off switch
// IOB7   power on switch

// IOC0   0x0001 pull 1  JP13 \ version jumper
// IOC1   0x0002 pull 1  JP12 |
// IOC2   0x0004 pull 1  JP11 |
// IOC3   0x0008 pull 1  JP10 /
// IOC4   0x0010 pull 1  JP14   logo
// IOC5   0x0020 pull 1  test point
// IOC6   0x0040 push 0  amp power
// IOC7   0x0080 push 1  system reset
// IOC8   0x0100 push 0  controller pin 8    controller select 1, high active (RTR)
// IOC9   0x0200 push 0  controller pin 5    controller select 2, high active (RTR)
// IOC10  0x0400 pull 1  controller pin 9    extint1, low active (RTS)
// IOC11  0x0800 push 0  N/C?
// IOC12  0x1000 pull 1  controller pin 6    extint2, low active (RTS)
// IOC13  0x2000 pull 0  controller pins 4 and 7 ANDed together, pullups (controller->system) (TX)
// IOC14  0x4000 push 0  controller pin 2, through a buffer (system->controller) (RX)
// IOC15  0x8000 push 1  power / charger / whatever enable (top left)

// idle state:
// IOC data=94bf push=cbc0 pull=343f special=6000

// extint1:
// IOC data=95bf push=cbc0 pull=343f special=6000 x16 (pull controller pin 8 high)

// extint2:
// IOC data=96bf push=cbc0 pull=343f special=6000 x16 (pull controller pin 5 high)

// *** doing IRQ 5
// IOC data=96bf push=cbc0 pull=343f special=6000 x6 (pull controller pin 5 high)

// (after no response from controller?)
// IOC data=94bf push=cbc0 pull=143f special=6000 (disable input on controller pin 4/7)
// IOC data=94bf push=ebc0 pull=143f special=6000 (enable output on controller pin 4/7)
// IOC data=94bf push=ebc0 pull=143f special=6000 (enable output on controller pin 4/7)
// IOC data=94bf push=ebc0 pull=143f special=0000
// IOC data=80bf push=ebc0 pull=003f special=0000 (disable input on extint1/2; extint1/2 low; disable interrupts?)
// IOC data=94bf push=ffc0 pull=003f special=0000 (enable output on extint1/2; extint1/2 high)
// IOC data=00bf push=ffc0 pull=003f special=0000 x11 (everything low; controller reset?)
// IOC data=20bf push=ffc0 pull=003f special=0000 (controller pin 4/7 high)
// IOC data=00bf push=dfc0 pull=203f special=0000 (controller pin 4/7 low; enable input on controller pin 4/7)
// IOC data=60bf push=dfc0 pull=203f special=0000 (controller pin 4/7/2 high)
// IOC data=00bf push=dfc0 pull=203f special=6000 (controller pin 4/7/2 low)
// IOC data=14bf push=dfc0 pull=203f special=6000 (extint1/2 high)
// IOC data=00bf push=cbc0 pull=343f special=6000 (extint1/2 low; disable output; enable input; enable interrupts?)

// controller:
// pin1      ground
// pin2 push system->controller (IOC14)  (RX)
// pin3      power
// pin4 pull controller->system (IOC13)  (TX)
// pin5 push (IOC9; controller select 2) (RTR)
// pin6 pull (IOC12; extint2) (RTS)
// pin7 pull controller->system (IOC13)  (TX)
// pin8 push (IOC8; controller select 1) (RTR)
// pin9 pull (IOC10; extint1) (RTS)

// 1=VCC (3V)
// 2=RTR (to request a report from the controller)
// 3=RX
// 4=GND
// 5=TX
// 6=RTS (to prepare the controller to change the state of an LED)

// controller messages:
// 55  : (controller->system) idle
// e4  : (system->controller) idle response 1
// e5  : (system->controller) idle response 2
// 60+X: (system->controller) set led: 8=red 4=yellow 2=blue 1=green 0=none
// 90+X: colour keys: 8=red 4=yellow 2=blue 1=green 0=release
// a0+X: other keys: 1=ok 2=quit 3=help 4=abc 0=release (1 for pen as well)
// 80+X: up: 3..7, 0=release stick (keypad uses 3)
// 88+X: down: 3..7
// c8+X: left: 3..7
// c0+X: right: 3..7
// b0+X: (system<->controller) sync

// idle:
//   ctl: 55
//   sys:    E4 D4 60
// sync:
//   sys: Ba                  Bb                  Bc                  ...
//   ctl:    (((0+a+F)&F)^B5)    (((a+b+F)&F)^B5)    (((b+c+F)&F)^B5) ...


#include <stdio.h>

#include "types.h"
#include "emu.h"
#include "platform.h"

#include "board.h"

#define V_X_GPIO_RTR1 0x0100
#define V_X_GPIO_RTR2 0x0200
#define V_X_GPIO_RTS1 0x0400
#define V_X_GPIO_RTS2 0x1000
#define V_X_GPIO_MISO 0x2000
#define V_X_GPIO_MOSI 0x4000

static int trace_gpio = 0;

static void init(void)
{
	if (mem[0x5675c] == 0x9311 && mem[0x5675e] == 0x4240 &&	// Winnie
	    mem[0x5675f] == 0x4e44)
		board->idle_pc = 0x5675c;

//	else
//		board->idle_pc = 0x4003a;	// studio, FIXME
}

static u32 bit(u16 x, u32 n)
{
	return (x >> n) & 1;
}

static u16 gpio(u32 n, u16 what, u16 push, u16 pull, u16 special)
{
//if (n == 2) what &= ~0x1400;
//if (n == 2) what |= 0x1400;

	if (trace_gpio) {
		static u16 old[3][4];

		u32 i;
		for (i = 0; i < (n == 1 ? 8 : 16); i++)
			if (bit(what, i) != bit(old[n][0], i)
			   || bit(push, i) != bit(old[n][1], i)
			   || bit(pull, i) != bit(old[n][2], i)
			   || bit(special, i) != bit(old[n][3], i))
				debug("IO%c%d data=%x push=%x pull=%x special=%x\n",
				       'A' + n, i, bit(what, i), bit(push, i), bit(pull, i), bit(special, i));

		old[n][0] = what;
		old[n][1] = push;
		old[n][2] = pull;
		old[n][3] = special;
	}

//	debug("IO%c data=%04x push=%04x pull=%04x special=%04x\n",
//	       'A' + n, what, push, pull, special);

	if (n == 2) {
		// system -> controller:
		//              ~75us ->||<-   ->|-|<- ~208us (4800bps)     P1     P2
		// 2 - RTR: ____________|"""""""""""""""""""|____________ 0x0100 0x0200 push 0 csel, active high output
		// 3 - RX : """""""""""""|_|"|_|"|_|"|_|"|_|""""""""""""" 0x4000 0x4000 push 0 system TX, controller RX
		// 6 - RTS: """"""""""""""""""""""""""""""""""""""""""""" 0x0400 0x1000 pull 1 extint, active low input
		//                        ^ ^ ^ ^ ^ ^ ^ ^ ^ ^
		//                    start 0 1 2 3 4 5 6 7 stop

		// controller -> system:
		//        ~10us ->||<-       ->|-|<- ~208us (4800bps)
		//               ->|-----------|<- ~3.7ms                    P1     P2
		// 2 - RTR: _______|"""""""""""""""""""""""""""""""|______ 0x0100 0x0200 push 0 csel, active high output
		// 5 - TX : """""""""""""""""""|_|"|_|"|_|"|_|"|_|"""""""" 0x2000 0x2000 pull 0 controller TX, system RX
		// 6 - RTS: """"""|____________|"""""""""""""""""""""""""" 0x0400 0x1000 pull 1 extint, active low input
		//                              ^ ^ ^ ^ ^ ^ ^ ^ ^ ^
		//                          start 0 1 2 3 4 5 6 7 stop

		printf("%d\n", get_video_line());

/*
		static u8 tx_buffer[256];
		static u8 tx_buf_start = 0;
		static u8 tx_buf_end = 0;
		static u8 tx_state = 0;

		static u8 sent = 0;
		if (button_A) {
			if (!sent) {
				tx_buffer[tx_buf_end++] = 0x55;
				sent = 1;
				printf("\nsent 55\n");
			}
		} else {
			sent = 0;
		}

		if (tx_buf_start == tx_buf_end) {
			// no transmit in progress
			tx_state = 0;
			what |= 0x2000;  // TX high
			what |= 0x0400;  // RTS high
			// if RTR is high, receive is in progress
			if (what & 0x0100) {
				printf("%c", (what & 0x4000) ? '"' : '_');  // RX
				fflush(stdout);
			}
		} else {
			// transmit in progress
			// wait for RTR to go high
			if (what & 0x0100) {
				if (tx_state < 18) {
					tx_state++;
					what |=  0x2000;  // TX high
					what &=~ 0x0400;  // RTS low
					return what;
				} else {
					switch (tx_state) {
						case 18: what &=~ 0x2000; break;  // start bit
						case 19: if (tx_buffer[tx_buf_start] & 0x01) what |= 0x2000; else what &=~ 0x2000; break;
						case 20: if (tx_buffer[tx_buf_start] & 0x02) what |= 0x2000; else what &=~ 0x2000; break;
						case 21: if (tx_buffer[tx_buf_start] & 0x04) what |= 0x2000; else what &=~ 0x2000; break;
						case 22: if (tx_buffer[tx_buf_start] & 0x08) what |= 0x2000; else what &=~ 0x2000; break;
						case 23: if (tx_buffer[tx_buf_start] & 0x10) what |= 0x2000; else what &=~ 0x2000; break;
						case 24: if (tx_buffer[tx_buf_start] & 0x20) what |= 0x2000; else what &=~ 0x2000; break;
						case 25: if (tx_buffer[tx_buf_start] & 0x40) what |= 0x2000; else what &=~ 0x2000; break;
						case 26: if (tx_buffer[tx_buf_start] & 0x80) what |= 0x2000; else what &=~ 0x2000; break;
						default: what |= 0x2000; break;  // stop bit
					}
					if ((tx_buf_start + 1) == tx_buf_end) {
						what |=  0x0400;  // RTS high (no more bytes)
					} else {
						what &=~ 0x0400;  // RTS low (more bytes)
					}
					if (tx_state > 26) {
						tx_buf_start++;
						tx_state = 0;
					} else {
						tx_state++;
					}
				}
			} else {
				tx_state = 0;
				what |=  0x2000;  // TX high
				what &=~ 0x0400;  // RTS low
				mem[0x3d22] |= 0x0200;  // trigger extint1
			}
		}
*/

		//printf("> IO%c data=%04x push=%04x pull=%04x special=%04x\n", 'A' + n, what, push, pull, special);

		/*
		static u8 button_state[12];
		static u8 serial_buffer[0x1000];
		static int serial_write_ptr;
		static int serial_read_ptr;
		static int serial_read_subptr;

		if ((what & 0xff00) == 0) {
			// reset
			for (int i = 0; i < 12; i++) button_state[i] = 0;
			serial_write_ptr = 0;
			serial_read_ptr = 0;
			serial_read_subptr = 0;
		}

		if (button_up != button_state[0]) {
			serial_buffer[(serial_write_ptr++) & 0xFFF] = ((button_state[0] = button_up) ? 0x83 : 0x80);
			mem[0x3d22] |= 0x0200;
		}
		if (button_down != button_state[1]) {
			serial_buffer[(serial_write_ptr++) & 0xFFF] = ((button_state[1] = button_down) ? 0x8B : 0x88);
			mem[0x3d22] |= 0x0200;
		}
		if (button_left != button_state[2]) {
			serial_buffer[(serial_write_ptr++) & 0xFFF] = ((button_state[2] = button_left) ? 0xCB : 0xC8);
			mem[0x3d22] |= 0x0200;
		}
		if (button_right != button_state[3]) {
			serial_buffer[(serial_write_ptr++) & 0xFFF] = ((button_state[3] = button_right) ? 0xC3 : 0xC0);
			mem[0x3d22] |= 0x0200;
		}
		if (
			button_red    != button_state[4] ||
			button_yellow != button_state[5] ||
			button_blue   != button_state[6] ||
			button_green  != button_state[7]
		) {
			serial_buffer[(serial_write_ptr++) & 0xFFF] = 0x90
				| ((button_state[4] = button_red   ) ? 8 : 0)
				| ((button_state[5] = button_yellow) ? 4 : 0)
				| ((button_state[6] = button_blue  ) ? 2 : 0)
				| ((button_state[7] = button_green ) ? 1 : 0);
			mem[0x3d22] |= 0x0200;
		}
		if (button_A != button_state[8]) { // ok
			serial_buffer[(serial_write_ptr++) & 0xFFF] = ((button_state[8] = button_A) ? 0xA1 : 0xA0);
			mem[0x3d22] |= 0x0200;
		} else if (button_menu != button_state[9]) { // quit
			serial_buffer[(serial_write_ptr++) & 0xFFF] = ((button_state[9] = button_menu) ? 0xA2 : 0xA0);
			mem[0x3d22] |= 0x0200;
		} else if (button_B != button_state[10]) { // help
			serial_buffer[(serial_write_ptr++) & 0xFFF] = ((button_state[10] = button_B) ? 0xA3 : 0xA0);
			mem[0x3d22] |= 0x0200;
		} else if (button_C != button_state[11]) { // abc
			serial_buffer[(serial_write_ptr++) & 0xFFF] = ((button_state[11] = button_C) ? 0xA4 : 0xA0);
			mem[0x3d22] |= 0x0200;
		}

		if (serial_read_ptr != serial_write_ptr) {
			if ((what & 0xff00) == 0x9500) {
				what &=~ 0x0400;
				serial_read_subptr = 1;
			} else if (serial_read_subptr) {
				switch (serial_read_subptr) {
					case  1: what &=~ 0x2000; serial_read_subptr++; break;
					case  2: if (serial_buffer[serial_read_ptr & 0xFFF] & 0x01) what |= 0x2000; else what &=~ 0x2000; serial_read_subptr++; break;
					case  3: if (serial_buffer[serial_read_ptr & 0xFFF] & 0x02) what |= 0x2000; else what &=~ 0x2000; serial_read_subptr++; break;
					case  4: if (serial_buffer[serial_read_ptr & 0xFFF] & 0x04) what |= 0x2000; else what &=~ 0x2000; serial_read_subptr++; break;
					case  5: if (serial_buffer[serial_read_ptr & 0xFFF] & 0x08) what |= 0x2000; else what &=~ 0x2000; serial_read_subptr++; break;
					case  6: if (serial_buffer[serial_read_ptr & 0xFFF] & 0x10) what |= 0x2000; else what &=~ 0x2000; serial_read_subptr++; break;
					case  7: if (serial_buffer[serial_read_ptr & 0xFFF] & 0x20) what |= 0x2000; else what &=~ 0x2000; serial_read_subptr++; break;
					case  8: if (serial_buffer[serial_read_ptr & 0xFFF] & 0x40) what |= 0x2000; else what &=~ 0x2000; serial_read_subptr++; break;
					case  9: if (serial_buffer[serial_read_ptr & 0xFFF] & 0x80) what |= 0x2000; else what &=~ 0x2000; serial_read_subptr++; break;
					default: what |= 0x2000; serial_read_subptr = 0; serial_read_ptr++; break;
				}
			}
		}
		*/

		// mem[0x3d22] |= 0x0200; // extint1 (p1?)
		// mem[0x3d22] |= 0x1000; // extint2 (p2?)
	}

	return what;
}

struct board board_V_X = {
	.use_centered_coors = 1,

	.init = init,
	.gpio = gpio
};
