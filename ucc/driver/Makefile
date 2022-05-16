CC          = gcc -m32
CFLAGS      = -g -DUCCDIR=\"$(shell pwd)/../bin/\"
all: ucc.c linux.c
	$(CC) -o ucc $(CFLAGS) $^

clean:
	rm -f *.o ucc

