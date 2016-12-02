/**********************************************************
	GCC
		sizeof(a) = 4 ,b.a = 30
		sizeof(struct A) = 4 
	Clang
		invalid application of 'sizeof' to an incomplete type
      'struct A'
        printf("sizeof(struct A) = %d \n",sizeof(struct A));
	UCC
		sizeof(a) = 4 ,b.a = 30
		sizeof(struct A) = 4 
	
 *********************************************************/
#include <stdio.h>
void f(struct A{int a;} b){
	b.a = 30;
	printf("sizeof(a) = %d ,b.a = %d\n",sizeof(b),b.a);
	printf("sizeof(struct A) = %d \n",sizeof(struct A));
}
typedef void (*FUNC_PTR)(void);
int main(){
	FUNC_PTR fptr = (FUNC_PTR)&f;
	fptr();
	/*printf("sizeof(struct A) = %d \n",sizeof(struct A));*/
	return 0;
}


