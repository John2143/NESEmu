CC=gcc -std=c99
CFLAGS=-Wall -Wextra -Iinclude
LDFLAGS=
FILES=main.c global.c assembly.c
SOURCES=$(addprefix src/, $(FILES))
EXECUTABLE=nes
LIBRARIES=

all:
	$(CC) $(CFLAGS) -g $(SOURCES) $(LIBRARIES) -o $(EXECUTABLE)

fast:
	$(CC) $(CFLAGS) -Ofast $(SOURCES) $(LIBRARIES) -o $(EXECUTABLE)

clean:
	rm *o
	rm *bin