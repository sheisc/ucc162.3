/****************************************
$ make ucc
ucc -o hello hello.c && ./hello
1.000000 2.000000 3.000000
0.000000 1.875000 0.000000
$ make clang
clang -std=c89 -o hello hello.c && ./hello
1065353216.000000 1073741824.000000 1077936128.000000
1.000000 2.000000 3.000000
$ make gcc
gcc -std=c89 -o hello hello.c && ./hello
2.000000 0.000000 2.852369
1.000000 2.000000 3.000000
 ****************************************/
#include <stdio.h>

void f(a,b,c)float a,b,c;{
	printf("%f %f %f\n",a,b,c);
}
void g(float a, float b,float c){
	printf("%f %f %f\n",a,b,c);
}
float arr[] = {1.0f,2.0f,3.0f};
int main(int argc,char * argv[]){
	int * ptr = (int *)arr;
	f(ptr[0],ptr[1],ptr[2]);
	f(1.0f,2.0f,3.0f);

	return 0;
}

