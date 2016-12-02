/***************************************************
VS2008:
	1>正在编译...
	1>main.c
	1>d:\projects\cppprojects\test\main.c(11) : error C2371: “DEBUG”: 重定义；不同的基类型
	1>        d:\projects\cppprojects\test\main.c(6) : 参见“DEBUG”的声明

GCC:
	hello.c: In function ‘test’:
	hello.c:11:21: error: conflicting types for ‘DEBUG’
	hello.c:6:20: note: previous declaration of ‘DEBUG’ was here
UCC:
	fails to report error


Bug:
	see	static char* GetAccessName(Symbol p)
	see 	CheckLocalDeclaration(AstDeclaration decl, Vector v)
 ***************************************************/
#include <stdio.h>
typedef struct {
    char *Istack[20];
} block_debug ;

extern block_debug DEBUG;
extern int a;
void test(void){
	typedef struct {
		 char *Istack[20];
	} block_debug ;
	extern block_debug DEBUG;
	extern double a;
	int i;	
	for(i = 0; i < 20; i++){
		DEBUG.Istack[i-1]= NULL;
	}
}
int main(){
	printf("main() begins. \n");
	test();
	printf("main() ends. \n");
	return 0;
}

