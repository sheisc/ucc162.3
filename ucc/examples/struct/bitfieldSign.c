/***********************************************
	see static Symbol ReadBitField(Field fld, Symbol p)
		in tranexpr.c
 ***********************************************/
#include <stdio.h>
typedef struct{
	int a;
	unsigned int b:1;
}Data;
Data dt;
int main(){
	dt.b = 1;
	printf("%d \n",dt.b);
	return 0;
}
