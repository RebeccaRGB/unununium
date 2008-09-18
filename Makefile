CC := gcc
CFLAGS := -std=gnu99 -Wall -W -Os -g $(shell sdl-config --cflags)
#CFLAGS := -Wall -W -O0 -g $(shell sdl-config --cflags)
LDFLAGS := -g

all: un-disas un-emu

un-disas: un-disas.o disas.o

un-emu: un-emu.o emu.o video.o disas.o sdl.o
	$(CC) $(LDFLAGS) -o $@ $^ $(shell sdl-config --libs)

*.o: *.h

clean:
	rm -f un-disas un-emu un-disas.o un-emu.o disas.o sdl.o emu.o video.o
