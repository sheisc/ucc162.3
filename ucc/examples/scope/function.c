#if 0
void f1(enum A a);
void g1(struct A);
#endif
#if 0
enum A f2(void);
struct A g2(void);
#endif
#if 0
void f3(struct A);
void f3(struct A);
#endif
struct A a;
void f(void){
	/*printf("%d \n",sizeof(a));*/
}
struct A{
	int arr[4];
};
int main(){
	printf("%d \n",sizeof(a));
	return 0;
}


