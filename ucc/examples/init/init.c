/************************************************
Bug:
See 
	static void CheckInitializer(AstInitializer init, Type ty)
 ************************************************/
int a = {{30}};
int main(int argc,char * argv[]){
	printf("%d \n",a);
	return 0;
}

