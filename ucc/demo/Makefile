all:	
	(ucc -E hello.c) || exit
	(ucl --dump-ast --dump-IR  hello.i) || exit
	(ucc -o hello hello.s -lm) || exit
	./hello	>	ucc.txt
	gcc -S -o hi.s hello.c -fno-asynchronous-unwind-tables
	((gcc -o hi hello.c -lm) && (./hi > gcc.txt)) || exit		
	cat ucc.txt
	diff ucc.txt gcc.txt
clean:
	rm -f hello.i hello.s hello.ast hello.uil hello a.out hi hi.s ucc.txt gcc.txt hello.asm
ucc89:
	ucc -o hello hello.c && ./hello
gcc89:
	gcc -std=c89 -o hello hello.c && ./hello
clang89:
	clang -std=c89 -o hello hello.c && ./hello
diff3:
	ucc -o hello hello.c;./hello
	gcc -std=c89 -o hello hello.c;./hello
	clang -std=c89 -o hello hello.c;./hello	
