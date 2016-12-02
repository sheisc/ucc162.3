#include <stdio.h>
/*******************************************************
$ make clang
clang -std=c89 -o hello hello.c && ./hello
$ make gcc
gcc -std=c89 -o hello hello.c && ./hello
hello.c: In function ‘main’:
hello.c:4:2: error: ‘for’ loop initial declarations are only allowed in C99 mode

 ******************************************************/
int main(int argc,char * argv[]){
	for(int i = 0; i < 5; i++){
	}
	
	return 0;
}
