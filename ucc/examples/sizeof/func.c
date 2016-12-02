/******************************************************
3.3.3.4 The sizeof operator
   The sizeof operator shall not be applied to an expression that has
function type or an incomplete type, to the parenthesized name of such
a type, or to an lvalue that designates a bit-field object.

But in GCC/Clang	,sizeof(function type) is 1.
 ******************************************************/
#include <stdio.h>
void f(void){
	printf("void f(void)\n");
}
int main(int argc,char * argv[]){

	printf("%d \n",sizeof(f));
	printf("%d \n",sizeof(void (void)));
	return 0;
}
