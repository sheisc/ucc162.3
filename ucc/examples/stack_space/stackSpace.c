#include <stdio.h>
/**************************************************
	UCC:
		arr2:	200  0xbfd59c98
		arr3:	300 0xbfd58c98
		arr1:	100 0xbfd5ac98
	GCC
		arr2:	200  0xbfe6b8c0
		arr3:	300 0xbfe6b8c0	---------- reuse the space of arr2
		arr1:	100 0xbfe6a8c0


 **************************************************/
void f(void){
	int arr1[1024];
	{
		int arr2[1024];
		arr2[0] = 200;
		printf("arr2:	%d  %p\n",arr2[0],&arr2[0]);
	}
	{
		int arr3[1024];
		arr3[0] = 300;
		printf("arr3:	%d %p\n",arr3[0],&arr3[0]);
	}
	arr1[0] = 100;
	printf("arr1:	%d %p\n",arr1[0],&arr1[0]);
}
int main(){
	f();
	return 0;
}

