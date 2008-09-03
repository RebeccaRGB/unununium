CC = gcc
CFLAGS = -Wall -W -O2
#CFLAGS = -Wall -W -g -O0

all: un-disas

un-disas: un-disas.o disas.o

clean:
	rm -f un-disas un-disas.o disas.o
