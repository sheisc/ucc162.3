/****************************************
(1)	UCC
(declchk.c,651):(hello.c,2):error:Identifier list should be in definition.
(2)	GCC
hello.c:2:1: warning: parameter names (without types) in function declaration [enabled by default]
(3)	Clang
hello.c:2:8: error: a parameter list without types is only allowed in a function
      definition
void f(a,b,c);
       ^
1 error generated.

 ****************************************/
#include <stdio.h>
void f(a,b,c);
int main(){
	return 0;
}

