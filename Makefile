CC = gcc
CFLAGS = -Wall -W -O2
#CFLAGS = -Wall -W -g -O0

all: un-disas un-emu

un-disas: un-disas.o disas.o
un-emu: un-emu.o disas.o

clean:
	rm -f un-disas un-emu un-disas.o un-emu.o disas.o
