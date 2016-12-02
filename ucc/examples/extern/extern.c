#include <stdio.h>

void test(void){	
	extern int arr[20];
	int i;	
	for(i = 0; i < 20; i++){
		arr[i-1]= 0;
	}
}

int main(){
	printf("main() begins. \n");
	test();
	printf("main() ends. \n");
	return 0;
}

