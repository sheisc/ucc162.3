/*************************************
	see 
		Symbol AddFunction(char *name, Type ty, int sclass)
		Symbol LookupFunctionID(char *name)
	in "symbol.c"
 *************************************/
#include <stdio.h>
void f(void){
#if 0	
	int k;
	void k(void);

#endif
#if 1
	
	void k(void);
	int k;
#endif
}
int main(int argc,char * argv[]){
	
	return 0;
}

