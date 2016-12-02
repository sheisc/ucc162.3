/*************************************
	see 
		void CheckLocalDeclaration(AstDeclaration decl, Vector v)
		static void CheckGlobalDeclaration(AstDeclaration decl)
	in "declchk.c"
 *************************************/
#include <stdio.h>
void f(int * ptr){
	printf("void f(int * ptr)\n");
}
void g(void){
	typedef void FUNC(int *);
	FUNC f;	/*	A function declaration here. Not a pointer variable */
	f(0);
	
}
int main(int argc,char * argv[]){
	g();
	return 0;
}

