CC = gcc

CFLAGS = -Wall -g -pedantic

all: minget minls

minget: minget.c minlib
	$(CC) $(CFLAGS) minget.c -o minget minlib

minls: minls.c minlib
	$(CC) $(CFLAGS) minls.c -o minls minlib

minlib: minlib.c
	$(CC) $(CFLAGS) minlib.c -c -o minlib

clean:
	rm *.o test* *.log