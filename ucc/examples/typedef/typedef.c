/**

Bug:
	see	static char* GetAccessName(Symbol p)
	see 	CheckLocalDeclaration(AstDeclaration decl, Vector v)
 */
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

