/************************************************
	for Bug in exprchk.c:
		AstExpression Adjust(AstExpression expr, int rvalue)
 ***********************************************/
struct Data{
	int arr[4];
	int a;
};
const struct Data dt = {
	1,2,3,4,5
};
typedef int ARRAY[4];
const ARRAY arr = {11,12,13,14};
int main(){
	arr[3] = 5;
	dt.arr[3] = 5;
	return 0;
}

