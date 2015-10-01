.PHONY: all clean

CC=gcc
CFLAGS=-Werror -Wall -Wextra -pedantic -std=c11

all: sim

sim: mips32.h elf.o sim.c
	$(CC) $(CFLAGS) -o sim elf.o sim.c

elf.o: elf.h elf.c
	$(CC) -c elf.c

clean:
	rm -f sim
	rm -f *.o
