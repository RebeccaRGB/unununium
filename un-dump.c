#include <stdio.h>

int main(void)
{
	int off, x, i;

	off = 0;

	for (;;) {
		printf("%06x:", off);
		off += 16;

		for (i = 0; i < 16; i++) {
			x = getchar();
			x |= getchar() << 8;
			printf(" %04x", x);
		}
		printf("\n");

		if (feof(stdin))
			exit(0);
	}
}
