# UCC

## A Lightweight Open-Source C Compiler for Research and Education

The original author is Wenjun Wang (Chief Architect of MIUI, Xiaomi Technology).

## Overview

### 1. The directory UCC contains the source code of the lightweight open-source C compiler (C89 standard).

#### How to build and use UCC 

```sh
iron@CSE:github$ pwd

    /home/iron/github

iron@CSE:github$ sudo apt-get install gcc-multilib g++-multilib

iron@CSE:github$ git clone https://github.com/sheisc/ucc162.3.git

iron@CSE:github$ cd ucc162.3/ucc

iron@CSE:ucc$ . ./ucc.sh



iron@CSE:ucc$ make

iron@CSE:ucc$ make install

mkdir -p  /home/iron/github/ucc162.3/ucc/bin
cp driver/ucc  /home/iron/github/ucc162.3/ucc/bin
cp ucl/ucl  /home/iron/github/ucc162.3/ucc/bin
cp ucl/assert.o  /home/iron/github/ucc162.3/ucc/bin
cp -r ucl/linux/include  /home/iron/github/ucc162.3/ucc/bin


iron@CSE:ucc$ make test

make -C ucl test 
make[1]: Entering directory '/home/iron/github/ucc162.3/ucc/ucl'
../driver/ucc -o ucl1 alloc.c ast.c decl.c declchk.c dumpast.c emit.c error.c expr.c exprchk.c flow.c fold.c gen.c input.c lex.c output.c reg.c simp.c stmt.c stmtchk.c str.c symbol.c tranexpr.c transtmt.c type.c ucl.c uildasm.c vector.c x86.c x86linux.c
mv  /home/iron/github/ucc162.3/ucc/bin/ucl  /home/iron/github/ucc162.3/ucc/bin/ucl.bak
cp ucl1  /home/iron/github/ucc162.3/ucc/bin/ucl
../driver/ucc -o ucl2 alloc.c ast.c decl.c declchk.c dumpast.c emit.c error.c expr.c exprchk.c flow.c fold.c gen.c input.c lex.c output.c reg.c simp.c stmt.c stmtchk.c str.c symbol.c tranexpr.c transtmt.c type.c ucl.c uildasm.c vector.c x86.c x86linux.c
mv  /home/iron/github/ucc162.3/ucc/bin/ucl.bak  /home/iron/github/ucc162.3/ucc/bin/ucl
strip ucl1 ucl2
cmp -l ucl1 ucl2
rm ucl1 ucl2
make[1]: Leaving directory '/home/iron/github/ucc162.3/ucc/ucl'


iron@CSE:ucc$ cd demo

iron@CSE:demo$ make ucc89

ucc -o hello hello.c && ./hello

iron@CSE:demo$ objdump -d ./hello



```

### 2. If you are interested in how to interpret a simplified C language, please refer to the directory MYC, which includes an interpreter.

#### How to build and use MYC
```sh
iron@CSE:github$ pwd

/home/iron/github

iron@CSE:github$ cd ucc162.3/myc/src

iron@CSE:src$ make

iron@CSE:src$ ./myc test.c

1 1 2 3 5 8 13 21 34 55 89 144 233 377 610 987 1597 2584 4181 

iron@CSE:src$ cat test.s

0:	 SetBx 1 0 0
1:	 Jmp 0 0 32
2:	 Init 4 1 0
3:	 Jeq 3 4 8
4:	 Jmp 0 0 5
5:	 Init 4 2 0
...


```






### 3. The directory ucc/examples/sc contains the code for Chapter 1 in the book (The Dissection of C Compiler.pdf).

#### How to build and use SC

```sh
iron@CSE:sc$ cd ~/github/

iron@CSE:github$ pwd

/home/iron/github

iron@CSE:github$ cd ucc162.3/ucc

iron@CSE:ucc$ . ./ucc.sh 

iron@CSE:ucc$ which ucc

/home/iron/github/ucc162.3/ucc/bin/ucc

iron@CSE:ucc$ cd examples/sc

iron@CSE:sc$ make

ucc -o sc lex.c expr.c error.c decl.c stmt.c main.c
cat demo.c
{
	int (*f(int,int,int))[4];
	int (*(*fp2)(int,int,int))(int);
	if(c)
		a = f;
	else{
		b = k;
	}

	while(c){
		while(d){
			if(e){
				d = d - 1;
			}
		}
		c = c - 1;
	}
}

./sc < demo.c
f is:  function(int,int,int) which returns pointer to array[4] of int 
fp2 is:  pointer to function(int,int,int) which returns pointer to function(int) which returns int 
	if(!c) goto Label_0 
	a = f 
	goto Label_1 
Label_0:
	b = k 
Label_1:
Label_2:
	if(!c) goto Label_6 
Label_3:
	if(!d) goto Label_5 
	if(!e) goto Label_4 
	t0 = d - 1 
	d = t0 
Label_4:
	goto Label_3 
Label_5:
	t1 = c - 1 
	c = t1 
	goto Label_2 
Label_6:

```

#### How to build and use SC2 (Generate 32-bit x86 assembly for Chapter 1 in the book)

```sh


iron@Katana:sc2$ make
gcc  -o sc lex.c expr.c error.c decl.c stmt.c main.c emit.c func.c
./sc < demo.c	> demo.s
gcc  -m32 demo.s -o demo

iron@Katana:sc2$ make run


iron@Katana:sc2$ cat demo.c

	
```











