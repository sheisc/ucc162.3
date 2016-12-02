/********************************************
	see static void PeepHole(BBlock bb)
			in simp.c
 ********************************************/
#if 1
#include <stdio.h>

int main(){
	int a = 26,b = 100;	
	a = a > 20 ? (a > 30? b+35: b+25): (b+15);
	printf("a = %d b = %d \n",a,b);
	return 0;
}
#endif
#if 0
#include <stdio.h>

int a = 1, b= 2;
int f(){
	printf("f()\n");
	return 10;
}
int g(){
	printf("g()\n");
	return 20;
}
int h(){
	printf("h()\n");
	return 30;
}
int * ptr = &b;
int main(){

	*ptr = a > 0 ? f(): b > 0 ? g():h();
	printf("%d \n",b);
	b = (a > 0 ? f(): b > 0) ? g():h();
	printf("%d \n",b);
	return 0;
}

#endif
