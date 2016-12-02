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
a = 3.0;		


int main(){
	//b = 3.0;		// ---------------	error	
	int c = 30;
	printf("%d \n",sizeof(a));
	return 0;
}

