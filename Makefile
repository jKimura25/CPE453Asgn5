CC = gcc

CFLAGS = -Wall -g -pedantic

all:
	$(CC) $(CFLAGS) minls.c
	$(CC) $(CFLAGS) minlib.c

clean:
	rm a.out