/************************************************
	(1)	GCC
				 error: initializer element is not computable at load time
	(2)	UCC
				error:Initializer must be constant
	(3)	Clang
				Pass,But the assembly output for @dt seems wrong.
 ************************************************/
struct Data{
	int a:31;
	char * b;
};
int b = "100"; 
struct Data dt = {4+"100"+3,&b+10};


int main(int argc,char * argv[]){
	
	return 0;
}

