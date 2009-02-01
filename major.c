#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>

#include "types.h"

int length;

static u8 *map_file(const char *name)
{
	int fd = open(name, O_RDONLY);
	length = lseek(fd, 0, SEEK_END);
	void *map = mmap(0, length, PROT_READ, MAP_SHARED, fd, 0);
	close(fd);
	return map;
}

int main(int argc, const char **argv)
{
	int n = argc - 1;
	u8 *data[n];
	int i, j;

	for (i = 0; i < n; i++)
		data[i] = map_file(argv[i + 1]);

	for (j = 0; j < length; j++) {
		int a[256];
		memset(a, 0, sizeof a);
		for (i = 0; i < n; i++)
			a[data[i][j]]++;
		int max = 0;
		u8 imax = 0;
		for (i = 0; i < 256; i++)
			if (a[i] > max) {
				max = a[i];
				imax = i;
			}
		write(1, &imax, 1);
		if (max < n-1) {
			fprintf(stderr, "warning: at least two outliers @ %06x:", j);
			for (i = 0; i < n; i++)
				fprintf(stderr, " %02x", data[i][j]);
			fprintf(stderr, " (picked %02x)\n", imax);
		}
	}

	return 0;
}

