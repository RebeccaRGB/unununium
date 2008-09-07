CC := gcc
CFLAGS := -Wall -W -O2 $(shell sdl-config --cflags)
#CFLAGS := -Wall -W -O0 -g $(shell sdl-config --cflags)
LDFLAGS := -lpng $(shell sdl-config --libs)

all: un-disas un-emu     page0 page1

un-disas: un-disas.o disas.o
un-emu: un-emu.o disas.o

clean:
	rm -f un-disas un-emu un-disas.o un-emu.o disas.o page0 page1
