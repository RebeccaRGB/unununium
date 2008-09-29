CC := gcc
CFLAGS := -std=gnu99 -Wall -W -Os -g
LDFLAGS := -g

# Build for what platform?
PLATFORM := sdl

# Default targets.
all: un-disas uuu-$(PLATFORM)

# The disassembler.
un-disas: un-disas.o disas.o

# The emulator.
uuu-$(PLATFORM): uuu-%: uuu-%.o platform-%.o emu.o video.o audio.o disas.o

# SDL needs special compiler flags and some libraries.
platform-sdl.o uuu-sdl.o: CFLAGS += $(shell sdl-config --cflags)
uuu-sdl: LDFLAGS += $(shell sdl-config --libs)

# Laziness rules, and lazy rules rule most of all.
*.o: *.h

# Clean up.
clean:
	rm -f un-disas un-disas.o disas.o emu.o video.o audio.o
	rm -f uuu-sdl uuu-sdl.o platform-sdl.o
