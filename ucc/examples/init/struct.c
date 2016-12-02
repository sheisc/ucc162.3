/****************************************************
see   CheckInitializerInternal
*****************************************************/
#include <stdio.h>
struct {
	int a;
	int b;
	struct{
		int c;
		int d;
	};
	int e;
}dt = {1,2,3,4,5};
int main(int argc,char * argv[]){	
	printf("%d \n",sizeof(dt));
	return 0;
}
