CC=gcc -g
CFLAGS=-W -Wall -Wextra -pedantic -O2

.PHONY: all clean

all: bfc.exe hello_world.exe

bfc.exe: brainfuck.o
	$(CC) -o bfc.exe $(CFLAGS) brainfuck.o

brainfuck.o: brainfuck.c
	$(CC) -c -o brainfuck.o $(CFLAGS) brainfuck.c

hello_world.exe: bfc.exe hello_world.bf
	bfc.exe hello_world.bf -o hello_world.exe

clean:
	del /f brainfuck.o bfc.exe
