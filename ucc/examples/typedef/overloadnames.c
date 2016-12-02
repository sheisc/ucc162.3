#include <stdio.h>

typedef const int CINT32;
void f(void){
	{
		double CINT32 = 1.23;
		{
			
		}
		/***************************************
			Because of the bug in void PostCheckTypedef(void)
			CINT32 is treated as a typedefed-name,not 
			a variable here.
			This causes the assertion fails in 
			declchk.c:: CheckDeclarationSpecifiers().
			That is,
				assert(sym->kind == SK_TypedefName);
		 ***************************************/
		printf("%d \n",sizeof(CINT32));
	}	
}

int main(){
	f();
	return 0;
}
