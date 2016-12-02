/*************************************
	see	static void CheckTypedef(AstDeclaration decl)
			in declchk.c
(1)	GCC/UCC:
			no warning/no error
(2)	Clang
			 error: redefinition of typedef 'INT32' is invalid in C
 *************************************/
#include <stdio.h>


/**********************************************
		(1) It's OK that two typedefs are the same.
			typedef	int	INT32;				
			typedef	int	INT32;	
		(2) Even the following is OK----------in C, it is OK.
						because we use 'struct INT32' in C.
						But in C++, it is not OK.
						Because in C++, we can use 'INT32' for the 'struct INT32'.
						So it is ambiguous for the compiler to know what 'INT32' means.
		
			typedef	int	INT32;
			typedef	int INT32;
			struct INT32{	---------------   because INT32 is not a typedefed-name
				int abc;
			};		
		(3) But these are illegal:
			typedef	int	INT32;
			typedef	int INT32;
			typedef struct {
				int abc;
			}INT32;		------------------  redeclaration of typedefed name INT32
		 **********************************************/
int main(int argc,char * argv[]){
	typedef int INT32;
	typedef int INT32;
	/**********************************
		we use "struct INT32" in C;
		"INT32" in C++.
		So for C++, "INT32" is ambiguous here.
 	 **********************************/
	struct INT32{	
		int arr[4];
	};
	return 0;
}

