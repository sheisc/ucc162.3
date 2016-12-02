/************************************************
	see	int CheckInitializerInternal()
			in declchk.c
 ************************************************/
struct Data{
	int a;
	char * b;
};
int b = "100"; 
struct Data dt = {4+"100"+3,&b+3};


int main(int argc,char * argv[]){
	
	return 0;
}

