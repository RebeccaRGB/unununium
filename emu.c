// Copyright 2008-2010  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include "types.h"
#include "disas.h"
#include "timer.h"
#include "video.h"
#include "audio.h"
#include "io.h"
#include "platform.h"
#include "board.h"

#include "emu.h"


#define FREQ 50
#define PERIOD (500000/FREQ)


u16 mem[N_MEM];

static u32 timer_handled;

static int trace = 0;
static int trace_new = 0;
static int store_trace = 0;
static int trace_calls = 0;
static int pause_after_every_frame = 0;
//static u8 trace_irq[9] = { 0, 1, 1, 1, 1, 1, 1, 1, 1 };
static u8 trace_irq[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static u8 unsp_version = 11;	// version a.b as 10*a + b
static u8 ever_ran_this[N_MEM];


static int do_extint1, do_extint2;


static u16 reg[8];
static u8 sb;
static u8 sb_banked[3];
static u8 irq_active, fiq_active;
static u8 irq_enabled, fiq_enabled;

static u64 insn_count;


u32 get_ds(void)
{
	return reg[6] >> 10;
}

void set_ds(u32 ds)
{
	reg[6] = (reg[6] & 0x03ff) | (ds << 10);
}

static void dump(u32 addr, u32 len)
{
	u32 off, i;

	for (off = addr & ~0x0f; off < addr + len; off += 0x10) {
		printf("%06x:", off);

		for (i = 0; i < 16; i++)
			printf(" %04x", mem[off+i]);
		printf("\n");
	}
}

static void store(u16 val, u32 addr)
{
	if (store_trace)
		printf("WRITE %04x TO %04x (was %04x)\n", val, addr, mem[addr]);

	if (addr < 0x2800)	// RAM
		mem[addr] = val;
	else if (addr < 0x3000)
		video_store(val, addr);
	else if (addr < 0x3800)
		audio_store(val, addr);
	else if (addr < 0x4000)
		io_store(val, addr);
	else
		printf("ROM STORE %04x to %04x\n", val, addr);
}

static u16 load(u32 addr)
{
	if (addr < 0x2800 || addr >= 0x4000)	// RAM / ROM
		return mem[addr];

	if (addr < 0x3000)
		return video_load(addr);

	if (addr < 0x3800)
		return audio_load(addr);

	return io_load(addr);
}

static u32 cs_pc(void)
{
	return ((reg[6] & 0x3f) << 16) | reg[7];
}

static void set_cs_pc(u32 x)
{
	reg[7] = x;
	reg[6] = (reg[6] & ~0x3f) | ((x >> 16) & 0x3f);
}

static void inc_cs_pc(s16 x)
{
	if (unsp_version < 11)
		reg[7] += x;
	else
		set_cs_pc(cs_pc() + x);
}

static u32 ds_reg(u8 r)
{
	return (get_ds() << 16) | reg[r];
}

static void set_ds_reg(u8 r, u32 x)
{
	reg[r] = x;
	set_ds((x & 0x3f0000) >> 16);
}

static void inc_ds_reg(u8 r, s16 x)
{
	if (unsp_version < 11)
		reg[r] += x;
	else
		set_ds_reg(r, ds_reg(r) + x);
}

static void push(u16 val, u8 b)
{
	store(val, reg[b]--);
}

static u16 pop(u8 b)
{
	return load(++reg[b]);
}

static void print_state(void)
{
	int i;

	printf("\n[insn_count = %llu]\n", insn_count);
	printf(" SP   R1   R2   R3   R4   BP   SR   PC   CS NZSC DS  SB IRQ FIQ\n");
	for (i = 0; i < 8; i++)
		printf("%04x ", reg[i]);
	printf(" %02x", reg[6] & 0x3f);
	printf(" %x%x%x%x", (reg[6] >> 9) & 1, (reg[6] >> 8) & 1,
	                    (reg[6] >> 7) & 1, (reg[6] >> 6) & 1);
	printf(" %02x", reg[6] >> 10);
	printf("  %x   %x   %x\n", sb, irq_enabled, fiq_enabled);
	disas(mem, cs_pc());
}

static void __do_irq(u16 vec)
{
	push(reg[7], 0);
	push(reg[6], 0);
	reg[7] = load(vec);
	reg[6] = 0;
}

static void do_irq(int irqno)
{
	if (fiq_active || irq_active || !irq_enabled)
		return;

	if (trace_irq[irqno])
		printf("### IRQ #%x ###\n", irqno);

	irq_active = 1;

	sb_banked[0] = sb;
	sb = sb_banked[1];
	__do_irq(0xfff8 + irqno);
	//sb = sb_banked[0];
}

static void do_fiq(void)
{
	if (fiq_active || !fiq_enabled)
		return;

	if (trace_irq[8])
		printf("### FIQ ###\n");

	fiq_active = 1;

	sb_banked[irq_active] = sb;
	sb = sb_banked[2];
	__do_irq(0xfff6);
	//sb = sb_banked[irq_active];
}

static int get_irq(void)
{
	// XXX FIQ

	if (!irq_enabled || irq_active)
		return -1;

	// video
	if (mem[0x2862] & mem[0x2863])
		return 0;

	// XXX audio, IRQ1

	// timerA, timerB
	if (mem[0x3d21] & mem[0x3d22] & 0x0c00)
		return 2;

	// UART, ADC		XXX: also SPI
	if (mem[0x3d21] & mem[0x3d22] & 0x2100)
		return 3;

	// XXX audio, IRQ4

	// extint1, extint2
	if (mem[0x3d21] & mem[0x3d22] & 0x1200)
		return 5;

	// 1024Hz, 2048HZ, 4096HZ
	if (mem[0x3d21] & mem[0x3d22] & 0x0070)
		return 6;

	// TMB1, TMB2, 4Hz, key change
	if (mem[0x3d21] & mem[0x3d22] & 0x008b)
		return 7;

	return -1;
}

static void maybe_enter_irq(void)
{
	int irqno = get_irq();

	if (irqno < 0)
		return;

	if (irqno == 8)
		do_fiq();
	else
		do_irq(irqno);
}

static void step(void)
{
	u16 op;
	u8 op0, opA, op1, opN, opB, opimm;
	u16 x0, x1;
	u32 old_cs_pc = cs_pc();
	u32 x, d = 0xff0000;

	if (trace_calls) {
		op = mem[cs_pc()];
		if ((op & 0xf3c0) == 0xf040 || (op & 0xfff7) == 0x9a90)
			print_state();
	}

	op = mem[cs_pc()];
	inc_cs_pc(1);


	// the top four bits are the alu op or the branch condition, or E or F
	op0 = (op >> 12);

	// the next three are usually the destination register
	opA = (op >> 9) & 7;

	// and the next three the addressing mode
	op1 = (op >> 6) & 7;

	// the next three can be anything
	opN = (op >> 3) & 7;

	// and the last three usually the second register (source register)
	opB = op & 7;

	// the last six sometimes are a single immediate number
	opimm = op & 63;


	// jumps
	if (opA == 7 && op1 < 2) {
		switch (op0) {
		case 0:		// JCC, JB, JNAE; C == 0
			if ((reg[6] & 0x40) == 0)
				goto do_jump;
			break;
		case 1:		// JCS, JNB, JAE; C == 1
			if ((reg[6] & 0x40) == 0x40)
				goto do_jump;
			break;
		case 2:		// JSC, JGE, JNL; S == 0
			if ((reg[6] & 0x80) == 0)
				goto do_jump;
			break;
		case 3:		// JSS, JNGE, JL; S == 1
			if ((reg[6] & 0x80) == 0x80)
				goto do_jump;
			break;
		case 4:		// JNE, JNZ; Z == 0
			if ((reg[6] & 0x0100) == 0)
				goto do_jump;
			break;
		case 5:		// JE, JZ; Z == 1
			if ((reg[6] & 0x0100) == 0x0100)
				goto do_jump;
			break;
		case 6:		// JPL; N == 0
			if ((reg[6] & 0x0200) == 0)
				goto do_jump;
			break;
		case 7:		// JMI; N == 1
			if ((reg[6] & 0x0200) == 0x0200)
				goto do_jump;
			break;
		case 8:		// JBE, JNA; not (Z == 0 && C == 1)
			if ((reg[6] & 0x0140) != 0x0040)
				goto do_jump;
			break;
		case 9:		// JNBE, JA; (Z == 0 && C == 1)
			if ((reg[6] & 0x0140) == 0x0040)
				goto do_jump;
			break;
		case 10:	// JLE, JNG; not (Z == 0 && S == 0)
			if ((reg[6] & 0x0180) != 0)
				goto do_jump;
			break;
		case 11:	// JNLE, JG; (Z == 0 && S == 0)
			if ((reg[6] & 0x0180) == 0)
				goto do_jump;
			break;
				// JVC: N == S
				// JVS: N != S
		case 14:	// JMP
			goto do_jump;
		default:
			goto bad;

		do_jump:
			if (op1 == 0)
				inc_cs_pc(opimm);
			else
				inc_cs_pc(-opimm);
		}
		return;
	}


	// PUSH
	if (op1 == 2 && op0 == 13) {
		while (opN--)
			push(reg[opA--], opB);
		return;
	}

	// POP
	if (op1 == 2 && op0 == 9) {
		// special case for RETI
		if (opA == 5 && opN == 3 && opB == 0) {
			reg[6] = pop(0);
			reg[7] = pop(0);

			if (fiq_active) {
				fiq_active = 0;
				sb_banked[2] = sb;
				sb = sb_banked[irq_active];
			} else if (irq_active) {
				irq_active = 0;
				sb_banked[1] = sb;
				sb = sb_banked[0];
			}

			maybe_enter_irq();

			return;
		}

		while (opN--)
			reg[++opA] = pop(opB);
		return;
	}


	if (op0 == 15) {
		switch (op1) {
		case 0:		// MUL US
			if (opN == 1) {
				if (opA == 7)
					goto bad;
				x = reg[opA]*reg[opB];
				if (reg[opB] & 0x8000)
					x -= reg[opA] << 16;
				reg[4] = x >> 16;
				reg[3] = x;
				return;
			} else
				goto bad;
		case 4:		// MUL SS
			if (opN == 1) {
				if (opA == 7)
					goto bad;
				x = reg[opA]*reg[opB];
				if (reg[opB] & 0x8000)
					x -= reg[opA] << 16;
				if (reg[opA] & 0x8000)
					x -= reg[opB] << 16;
				reg[4] = x >> 16;
				reg[3] = x;
				return;
			} else
				goto bad;
		case 1:		// CALL
			if ((opA & 1) != 0)
				goto bad;
			x1 = mem[cs_pc()];
			inc_cs_pc(1);
			push(reg[7], 0);
			push(reg[6], 0);
			set_cs_pc((opimm << 16) | x1);
			return;
		case 2:		// JMPF
			if (opA != 7)
				goto bad;
			x1 = mem[cs_pc()];
			set_cs_pc((opimm << 16) | x1);
			return;
		case 5:		// VARIOUS
			switch (opimm) {
			case 0:
				irq_enabled = 0;
				fiq_enabled = 0;
				//printf("INT OFF\n");
				return;
			case 1:
				irq_enabled = 1;
				fiq_enabled = 0;
				//printf("INT IRQ\n");
				return;
			case 2:
				irq_enabled = 0;
				fiq_enabled = 1;
				//printf("INT FIQ\n");
				return;
			case 3:
				irq_enabled = 1;
				fiq_enabled = 1;
				//printf("INT FIQ,IRQ\n");
				return;
			case 8:
				irq_enabled = 0;
				//printf("IRQ OFF\n");
				return;
			case 9:
				irq_enabled = 1;
				//printf("IRQ ON\n");
				return;
			case 12:
				fiq_enabled = 0;
				//printf("FIQ OFF\n");
				return;
			case 14:
				fiq_enabled = 1;
				//printf("FIQ ON\n");
				return;
			case 0x25:	// NOP
				return;
			default:
				goto bad;
			}
		default:
			goto bad;
		}
	}


	// alu op

	// first, get the arguments
	x0 = reg[opA];

	switch (op1) {
	case 0:		// [BP+imm6]
		d = (u16)(reg[5] + opimm);
		if (op0 == 13)
			x1 = 0x0bad;
		else
			x1 = load(d);
		break;
	case 1:		// imm6
		x1 = opimm;
		break;
	case 3:		// [Rb] and friends
		if (opN & 4) {
			if ((opN & 3) == 3)
				inc_ds_reg(opB, 1);
			d = ds_reg(opB);
			if (op0 == 13)
				x1 = 0x0bad;
			else
				x1 = load(d);
			if ((opN & 3) == 1)
				inc_ds_reg(opB, -1);
			if ((opN & 3) == 2)
				inc_ds_reg(opB, 1);
		} else {
			if ((opN & 3) == 3)
				reg[opB]++;
			d = reg[opB];
			if (op0 == 13)
				x1 = 0x0bad;
			else
				x1 = load(d);
			if ((opN & 3) == 1)
				reg[opB]--;
			if ((opN & 3) == 2)
				reg[opB]++;
		}
		break;
	case 4:
		switch(opN) {
		case 0:		// Rb
			x1 = reg[opB];
			break;
		case 1:		// imm16
			x0 = reg[opB];
			x1 = mem[cs_pc()];
			inc_cs_pc(1);
			break;
		case 2:		// [imm16]
			x0 = reg[opB];
			d = mem[cs_pc()];
			inc_cs_pc(1);
			if (op0 == 13)
				x1 = 0x0bad;
			else
				x1 = load(d);
			break;
		case 3:		// [imm16] = ...
			x0 = reg[opB];
			x1 = reg[opA];
			d = mem[cs_pc()];
			inc_cs_pc(1);
			break;
		default:	// ASR
			{
				u32 shift = (reg[opB] << 4) | sb;
				if (shift & 0x80000)
					shift |= 0xf00000;
				shift >>= (opN - 3);
				sb = shift & 0x0f;
				x1 = (shift >> 4) & 0xffff;
			}
		}
		break;
	case 5:
		if (opN < 4) {	// LSL
			u32 shift = ((sb << 16) | reg[opB]) << (opN + 1);
			sb = (shift >> 16) & 0xf;
			x1 = shift & 0xffff;
		} else {	// LSR
			u32 shift = ((reg[opB] << 4) | sb) >> (opN - 3);
			sb = shift & 0x0f;
			x1 = (shift >> 4) & 0xffff;
		}
		break;
	case 6:
		if (opN < 4) {	// ROL
			u32 shift = ((((sb << 16) | reg[opB]) << 4) | sb) << (opN + 1);
			sb = (shift >> 20) & 0x0f;
			x1 = (shift >> 4) & 0xffff;
		} else {	// ROR
			u32 shift = ((((sb << 16) | reg[opB]) << 4) | sb) >> (opN - 3);
			sb = shift & 0x0f;
			x1 = (shift >> 4) & 0xffff;
		}
		break;
	case 7:
		x1 = load(opimm);
		break;
	default:
		goto bad;
	}

//printf("--> args: %04x %04x\n", x0, x1);

	// then, perform the alu op
	switch (op0) {
	case 0:		// ADD
		x = x0 + x1;
		break;
	case 1:		// ADC
		x = x0 + x1;
		if (reg[6] & 0x40)
			x++;
		break;
	case 2: case 4:	// SUB, CMP
		x = x0 + (~x1 & 0xffff) + 1;
		break;
	case 3:		// SBC
		x = x0 + (~x1 & 0xffff);
		if (reg[6] & 0x40)
			x++;
		break;
	case 6:		// NEG
		x = -x1;
		break;
	case 8:		// XOR
		x = x0 ^ x1;
		break;
	case 9:		// LOAD
		x = x1;
		break;
	case 10:	// OR
		x = x0 | x1;
		break;
	case 11: case 12: // AND, TEST
		x = x0 & x1;
		break;
	case 13:	// STORE
		store(x0, d);
		return;
	default:
		goto bad;
	}

	// set N and Z flags
	if (op0 < 13) {		// not STORE
		reg[6] = (reg[6] & ~0x0300);

		if (x & 0x8000)
			reg[6] |= 0x0200;

		if ((x & 0xffff) == 0)
			reg[6] |= 0x0100;
	}

	// set S and C flags
	if (op0 < 5) {		// ADD, ADC, SUB, SBC, CMP
		reg[6] = (reg[6] & ~0x00c0);

		if (x != (u16)x)
			reg[6] |= 0x40;

		// this only works for cmp, not for add (or sbc).
		if ((s16)x0 < (s16)x1)
			reg[6] |= 0x80;
	}

	if (op0 == 4 || op0 == 12)	// CMP, TEST
		return;

//printf("--> result: %04x\n", x);

	if (op1 == 4 && opN == 3)	// [imm16] = ...
		store(x, d);
	else
		reg[opA] = x;

	return;


bad:
	set_cs_pc(old_cs_pc);
	print_state();
	fatal("! UNIMPLEMENTED\n");
}

static u32 last_retrace_time = 0;

static void do_idle(void)
{
	u32 now;
	struct timeval tv;

//	printf("### IDLE ###\n");

	gettimeofday(&tv, 0);
	now = 1000000*tv.tv_sec + tv.tv_usec;
	if (now < last_retrace_time + PERIOD) {
//		printf("  sleeping %dus\n", last_retrace_time + PERIOD - now);
		usleep(last_retrace_time + PERIOD - now);
	}
}

u16 get_video_line(void)
{
	u32 now;
	struct timeval tv;

	gettimeofday(&tv, 0);
	now = 1000000*tv.tv_sec + tv.tv_usec;
	return (now - last_retrace_time) * 625 * FREQ / 2 / 1000000;	// 525 for NTSC
}

static void run_main(void)
{
	int idle = 0;

	for (;;) {
		if (timer_triggered != timer_handled)
			break;

		if (trace)
			print_state();

		if (trace_new && ever_ran_this[cs_pc()] == 0) {
			ever_ran_this[cs_pc()] = 1;
			print_state();
		}

		if (cs_pc() == board->idle_pc) {
			idle++;
			if (idle == 2)
				break;
		}

		step();
		insn_count++;
	}

	if (timer_triggered != timer_handled) {
		timer_handled++;
		timer_set();
	} else
		do_idle();
}

static void do_controller(void)
{
	char key;

	do {
		key = update_controller();

		switch (key) {
		case 0x1b:
			printf("Goodbye.\n");
			exit(0);

		case '0' ... '7':
			printf("*** doing IRQ %c\n", key);
			do_irq(key - '0');
			break;

		case '8':
			printf("*** doing FIQ\n");
			do_fiq();
			break;

		case 't':
			trace ^= 1;
			break;

		case 'y':
			trace_new ^= 1;
			break;

		case 'u':
			pause_after_every_frame ^= 1;
			break;

		case 'v':
			dump(0x2800, 0x80);
			break;

		case 'c':
			dump(0x2900, 0x300);
			break;

		case 'q':
			mute_audio ^= 1;
			break;

		case 'w':
			do_extint1 = 1;
			break;

		case 'e':
			do_extint2 = 1;
			break;

		case 'a':
			hide_page_1 ^= 1;
			break;

		case 's':
			hide_page_2 ^= 1;
			break;

		case 'd':
			hide_sprites ^= 1;
			break;

		case 'x':
			dump(0, 0x4000);
			break;
		}
	} while (key);
}

static void run(void)
{
	run_main();

	struct timeval tv;
	u32 now;

	gettimeofday(&tv, 0);
	now = 1000000*tv.tv_sec + tv.tv_usec;

	if (now - last_retrace_time >= PERIOD) {
		static int count = 0;
		count++;
		if (count == 5) {
			screen_needs_update = 1;
			count = 0;
		}

		// video
		// FIXME: make this better
		static u32 which = 1;

		mem[0x2863] |= which;
		which ^= 3;

		last_retrace_time = now;

		do_controller();

		mem[0x3d22] |= 2;	// TMB2		FIXME: freq

		if (do_extint1) {
			mem[0x3d22] |= 0x0200;
			do_extint1 = 0;
		}
		if (do_extint2) {
			mem[0x3d22] |= 0x1000;
			do_extint2 = 0;
		}

		// UART		FIXME
mem[0x3d22] |= 0x0100;
	}

	maybe_enter_irq();


#if 0
// XXX: move this elsewhere
	if (pause_after_every_frame) {
		printf("*** paused, press a key to continue\n");

		while (update_controller() == 0)
			;
	}
#endif
}

void emu(void)
{
	platform_init();
	read_rom(0);
	board_init();
	audio_init();
	timer_init();

	memset(reg, 0, sizeof reg);
	reg[7] = mem[0xfff7];	// reset vector

	for (;;)
		run();
}
