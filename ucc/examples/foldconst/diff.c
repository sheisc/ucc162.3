/**********************************************
	see fold.c
 **********************************************/
#include <stdio.h>
typedef struct{
	int holder;
	int isarray:31;
	int b;
}Data;
Data dt;
int a;
int b = 32;
int main(){
#if 0
	a = dt.isarray = 11;
	printf("%d \n",a);
	a = dt.isarray = -1;
	printf("%d \n",a);
#endif
#if 1
	printf("%x\n",(1<<b));
	/*********************************************
		Warning
		The following statements have different results 
		even in GCC.
		When GCC do foldconsts, (1<<32) is evaluated to 0.
								 (1<<b) is evaluated at run time.
		But for UCC, when ucc do foldconsts, 1 and 32 are in 
			variables for GCC.(Because we use GCC to compiles UCC sourcecode).
		In fact, when ucc do foldconsts,
			we could do some more checks to be consistent with GCC.
		
		According to c89,
		"If the value
of the right operand is negative or is greater than or equal to the
width in bits of the promoted left operand, the behavior is undefined."
 	 *********************************************/
	printf("%x\n",(1<<b)-1);	
	printf("%x\n",(1<<32)-1);	
	printf("%x\n",(8<<-1)-1);
	printf("%x\n",(1>>32));
#endif
	return 0;
}

					
