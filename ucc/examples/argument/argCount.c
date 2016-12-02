/*********************************************
	see 
		AstExpression CheckFunctionCall()
		AstExpression CheckArgument()
 *********************************************/
#include <stdio.h>

void f(a,b,c)float a,b,c;{
}
void g(void){
	
}

int main(int argc,char * argv[]){
	f(1,2,3);
	f(1,2);
	f(1,2,3,4);
	g();
	g(1);
	return 0;
}


