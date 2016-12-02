#include <stdio.h>
/*********************************************************
clang -o hello hello.c;./hello
0xbfe5ce50 0xbfe5ce48

gcc -o hello hello.c;./hello
0xbfb498d0 0xbfb498d0

ucc -o hello hello.c;./hello
0xbf8aded4 0xbf8aded8

 *********************************************************/
typedef struct {

}Data;

void f(Data dt1,Data dt2){
	dt1 = dt2;
	printf("%p %p\n",&dt1,&dt2);
}
int main(){
	Data d2;
	Data d1;
	d1 = d2;
	f(d1,d2);

	return 0;
}
