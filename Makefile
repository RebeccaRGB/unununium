include config


CFLAGS += -std=gnu99 -Wall -W -O2 -g  -Wmissing-declarations -ffast-math
CFLAGS += -DRENDER_$(RENDER)
LDFLAGS := -g

ifeq ($(USE_DEBUG),yes)
CFLAGS += -DUSE_DEBUG
endif


# Default targets.
.PHONY: all
all: un-disas uuu-$(PLATFORM)

# The disassembler.
un-disas: un-disas.o disas.o

# The emulator.
uuu-$(PLATFORM): uuu-%: uuu-%.o platform-%.o \
                 disas.o emu.o timer.o video.o render.o render-$(RENDER).o \
                 audio.o io.o i2c-bus.o i2c-eeprom.o board.o \
                 board-VII.o board-WAL.o board-BAT.o board-V_X.o board-dummy.o

# Is this compiler targetting MacOSX?
is-osx = $(shell set -e; \
        if ($(CC) -E -dM - | grep __APPLE__) </dev/null >/dev/null 2>&1; \
        then echo "yes" ; fi; )

# SDL needs special compiler flags and some libraries.
platform-sdl.o uuu-sdl.o: CFLAGS += $(shell sdl-config --cflags)
uuu-sdl: LDFLAGS += $(shell sdl-config --libs)

ifeq ($(RENDER),gl)
ifeq ($(is-osx),yes)
uuu-sdl: LDFLAGS += -Wl,-framework,OpenGL
else
uuu-sdl: LDFLAGS += -lGL
endif
endif

# If MacOSX, use Cocoa dialogs, and create an application bundle.
ifeq ($(is-osx),yes)
uuu-sdl: dialog-cocoa.o

%.o: %.m
	$(CC) $(CFLAGS) -o $@ -c $<

all: .stamp-bundle

.stamp-bundle: uuu-sdl
	-mkdir -p Unununium.app/Contents/Resources/ROMs
	-mkdir -p Unununium.app/Contents/MacOS
	cp uuu-sdl Unununium.app/Contents/MacOS/
	touch $@
endif

# Laziness rules, and lazy rules rule most of all.
*.o: *.h Makefile config

# Clean up.
.PHONY: clean
clean:
	rm -f un-disas un-disas.o disas.o
	rm -f emu.o timer.o video.o audio.o io.o
	rm -f i2c-bus.o i2c-eeprom.o
	rm -f board.o board-VII.o board-WAL.o board-BAT.o board-V_X.o board-dummy.o
	rm -f render.o render-gl.o render-soft.o
	rm -f uuu-sdl uuu-sdl.o platform-sdl.o dialog-cocoa.o
