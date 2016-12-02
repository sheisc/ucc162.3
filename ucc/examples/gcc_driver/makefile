#	make -s
all:
	/usr/bin/cpp hello.c -o hello.i
	/usr/lib/gcc/i686-linux-gnu/4.6/cc1 -quiet hello.i -o hello.s
	/usr/bin/as hello.s -o hello.o
	/usr/bin/ld --eh-frame-hdr -m elf_i386 --hash-style=gnu -dynamic-linker /lib/ld-linux.so.2 -z relro -o hello /usr/lib/gcc/i686-linux-gnu/4.6/../../../i386-linux-gnu/crt1.o /usr/lib/gcc/i686-linux-gnu/4.6/../../../i386-linux-gnu/crti.o /usr/lib/gcc/i686-linux-gnu/4.6/crtbegin.o -L/usr/lib/gcc/i686-linux-gnu/4.6 -L/usr/lib/gcc/i686-linux-gnu/4.6/../../../i386-linux-gnu -L/usr/lib/gcc/i686-linux-gnu/4.6/../../../../lib -L/lib/i386-linux-gnu -L/lib/../lib -L/usr/lib/i386-linux-gnu -L/usr/lib/../lib -L/usr/lib/gcc/i686-linux-gnu/4.6/../../.. hello.o -lgcc --as-needed -lgcc_s --no-as-needed -lc -lgcc --as-needed -lgcc_s --no-as-needed /usr/lib/gcc/i686-linux-gnu/4.6/crtend.o /usr/lib/gcc/i686-linux-gnu/4.6/../../../i386-linux-gnu/crtn.o -lc

clean:
	rm -f hello.i hello.s hello.o hello

