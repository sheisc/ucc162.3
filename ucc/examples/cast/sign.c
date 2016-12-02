/**************************************************
	see AstExpression Cast(Type ty, AstExpression expr)
		in exprchk.c
		Symbol TranslateCast(Type ty, Type sty, Symbol src)
		in tranexpr.c	
 **************************************************/
#include <stdio.h>
short a = -1;
int main(int argc,char * argv[]){
	int b ;
#if 1
	b = (int)(((unsigned int)a) >> 1);
	printf("%x \n",b);
#endif
#if 1
	b = (((unsigned int)a) >> 1);
	printf("%x \n",b);
#endif
#if 1
	printf("%x \n",(((unsigned int)a) >> 1));
#endif
	return 0;
}

