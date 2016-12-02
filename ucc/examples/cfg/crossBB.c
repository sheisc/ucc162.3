/*****************************************************************
	See EmitBBlock(), EmitBranch()
		and 
		This bug is found by testing "Livermore loops".
		See	Line 1567 in cflops.c
		ucc -DREAL=float --dump-ast --dump-IR -S cflops.c
 *****************************************************************/
#include <stdio.h>
#define abs(x) ((x)>=0?(x):-(x))
double stat[72];
double ox[72];
double x[72];
/************************************************************
	see the result of UIL .
	Some temporaries containing address are used accross Basic blocks,
	which is quite amazing.
 ************************************************************/
void statw( double stat[],double ox[],double x[]){
	ox[2] = abs(x[2]-stat[2]);
}
double number = 1.23, result;
int main(){
	statw(stat,ox,x);
	/************************************************************
		For conditional-expression, 
		some temporaries are also used across basic blocks.
	*************************************************************/
	result = abs(number);
	return 0;
}

