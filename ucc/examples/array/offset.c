/***********************************************
	see CheckUnaryExpression(AstExpression expr) in exprchk.c
 ***********************************************/
#include <stdio.h>
int arr[4][4];
typedef	int ARRAY[4][4];
ARRAY * ptr = &arr;
ARRAY ** ptr2 = &ptr;
int main(){
	int index = 2;

#if 1
	**arr = 5;
	printf("%d \n",arr[0][0]);
	(*arr)[2] = 10;
	printf("%d \n",arr[0][2]);
	arr[index][2] = 20;
	printf("%d \n",arr[2][2]);
	(*ptr)[2][2] = 30;
	printf("%d \n",arr[2][2]);
#endif
	(**ptr2)[2][2] = 40;
#if 1
	printf("%d \n",arr[2][2]);
#endif
	return 0;
}
