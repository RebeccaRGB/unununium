#include <stdio.h>

typedef unsigned char u8;
typedef unsigned short u16;

u16 transp = 0x7c1f;

u8 mem[0x800000];
u8 palette[3][257];

static u16 get16(int i)
{
	return mem[2*i] + 256*mem[2*i+1];
}

int main(void)
{
	int x0, y0, x1, y1, c;

	fread(mem, 1, sizeof mem, stdin);

	printf("P6 %d %d 255\n", 320, 240);

	for (c = 0; c <= 256; c++) {
		u16 pal;

		if (c == 256)
			pal = transp;
		else
			pal = get16(0x1feb00+c);
		if (pal & 0x8000)
			pal = transp;
		palette[0][c] = (pal >> 7) & 0xf8;
		palette[1][c] = (pal >> 2) & 0xf8;
		palette[2][c] = (pal << 3) & 0xf8;
	}

	for (y0 = 0; y0 < 15; y0++)
		for (y1 = 0; y1 < 16; y1++)
			for (x0 = 0; x0 < 20; x0++)
				for (x1 = 0; x1 < 16; x1++) {
					int c;
					u16 tile = get16(0x1fe800 + x0 + 32*y0);

					if (tile == 0)
						c = 256;
					else {
						int addr = 2*0x139140 + 256*tile;
						c = mem[addr + 16*y1 + x1];
					}
					putchar(palette[0][c]);
					putchar(palette[1][c]);
					putchar(palette[2][c]);
				}

	return 0;
}
