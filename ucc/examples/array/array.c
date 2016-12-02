/*****************************************************
	see CheckPostfixExpression(AstExpression expr) in exprchk.c
 *****************************************************/
#include <stdio.h>
char * str[] = {
	"11111111",
	"22222222",
	"33333333"
};
double a = 1.0;
double b = 2.0;
double c = 3.0;
double * fps[3][4] = {
	&a,
	&b,
	&c,
	&a,
	&b,
	&c,
	&a,
	&b,
	&c,
	&a,
	&b,
	&c,
};
typedef double * (* FP64_ARR)[4];

int main(){
	FP64_ARR ptr = fps;
	printf("%x %x\n",str[1][2],*(str[1]+2));
	printf("%f %f\n",fps[1][2][0],*(fps[1][2]));
	printf("%f %f\n",ptr[1][2][0],*(ptr[1][2]));
	return 0;
}

