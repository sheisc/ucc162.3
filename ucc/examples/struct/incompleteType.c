#include <stdio.h>
typedef struct{
	int a;
	struct{
	}b;
	int c[];
} Data;
struct A;	//	It is considered an incomplete type here. So sizeof(struct A) is illegal.
void f(int arr[]){
	Data d;
	printf("%d \n",sizeof(d.b));
	printf("%d \n",sizeof(Data));
	printf("%d \n",sizeof(arr));
	// The following are incomplete types:
	//printf("sizeof(struct A) = %d  \n",sizeof(struct A));
	//printf("%d \n",sizeof(d.c));
}
struct A{
	int a[10];
};
int main(){
	int i = 0;
	f(0);
	printf("sizeof(struct A) = %d  \n",sizeof(struct A));
	return 0;
}

