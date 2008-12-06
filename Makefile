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
uuu-$(PLATFORM): uuu-%: uuu-%.o platform-%.o emu.o video.o audio.o io.o disas.o

# SDL needs special compiler flags and some libraries.
platform-sdl.o uuu-sdl.o: CFLAGS += $(shell sdl-config --cflags)
uuu-sdl: LDFLAGS += $(shell sdl-config --libs)

# Is this compiler targetting MacOSX?
is-osx = $(shell set -e; \
        if ($(CC) -E -dM - | grep __APPLE__) </dev/null >/dev/null 2>&1; \
        then echo "yes" ; fi; )

# If MacOSX, use Cocoa dialogs, and create an application bundle.
ifeq ($(is-osx),yes)
uuu-sdl: dialog-cocoa.o

all: bundle

bundle: uuu-sdl
	-mkdir -p Unununium.app/Contents/MacOS
	cp uuu-sdl Unununium.app/Contents/MacOS/
endif

# Laziness rules, and lazy rules rule most of all.
*.o: *.h

# Clean up.
clean:
	rm -f un-disas un-disas.o disas.o emu.o video.o audio.o io.o
	rm -f uuu-sdl uuu-sdl.o platform-sdl.o dialog-cocoa.o
