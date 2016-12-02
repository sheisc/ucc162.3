/**************************************************
	see static void EliminateCode(BBlock bb)
		in simp.c	
 **************************************************/
typedef struct{
	int arr[4];
}Data;
Data GetData(void){
	Data dt;
	return dt;
}
int f(void){
	return 30;
}
int main(int argc,char * argv[]){
	GetData();
	f();
	return 0;
}

