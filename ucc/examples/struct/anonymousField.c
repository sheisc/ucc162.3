#include <stdio.h>
typedef struct{
	struct{
		int;
		double;
	};
	union{
		int;
		double;
	};
	double ;
	int d;
}Data;
Data dt[10];
int main(){
	
	
	printf("%p %p %d\n",&dt[0],&dt[9],sizeof(dt[0]));
	return 0;
}

