/*****************************************
	For Bug	in
		static int CheckEnumerator(AstEnumerator enumer, int last, Type ty)
 *****************************************/
#include <stdio.h>

enum Color{
	Red,
};
enum Color2{
	Red,
};

int main(){
	printf("%d \n",sizeof(enum Color));
	return 0;
}

