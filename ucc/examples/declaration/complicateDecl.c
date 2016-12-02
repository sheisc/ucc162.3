#include <stdio.h>
/*******************************************
	Trigger a bug in UCC 
		in GetFunctionDeclarator(AstInitDeclarator initDec)
 **********************************************/
int (* f(void))(void){
	return 0;
}
/**********************************************
	From <Thinking in C++> V1 Chapter3: The C in C++
	Complicated declarations & definitions:
	
 **********************************************/
/* 1. */     void * (*(*fp1)(int))[10];

/* 2. */     float (*(*fp2)(int,int,float))(int);

/* 3. */     typedef double (*(*(*fp3)())[10])();
             fp3 a;

/* 4. */     int (*(*f4())[10])();
int main(){

	printf("hello world.\n");
	return 0;
}

