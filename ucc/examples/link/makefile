UCC_SRC_DIR = ~/src/ucc
CC = gcc
CFLAGS = -DREAL_T=double 
#-Werror=implicit-function-declaration
#missing-declarations
#strict-prototypes
#-fno-asynchronous-unwind-tables
SRC_C	= 1.c 2.c
all: 1.c 2.c
	$(CC)	$(CFLAGS) -c $(SRC_C)
	$(CC)	$(CFLAGS) -S $(SRC_C)
	$(CC)	-o hello 1.o 2.o
	./hello
	#objdump -D ./hello	>hello.asm
clean:
	rm -f 1.o 2.o hello
