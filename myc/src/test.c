//fibonacci
int fibonacci(int n){
	if(n == 1 || n == 2)
		return 1;
	else
		return fibonacci(n-1)+fibonacci(n-2);
}
//main
int main(){
	int count = 1;
	while(count < 20){
		printf(fibonacci(count));
		count = count + 1;
	}
}

