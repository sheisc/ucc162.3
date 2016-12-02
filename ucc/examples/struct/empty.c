#include <stdio.h>

typedef struct {

}Data;
Data dt[10];
int main(){
	
	printf("%p %p %d %d\n",&dt[0],&dt[9],sizeof(dt),sizeof(Data));
	return 0;
}
