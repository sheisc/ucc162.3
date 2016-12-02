/******************************************************************
	Bug
	see symbol.c	CreateOffset(...)
 ******************************************************************/
#include <stdio.h>
typedef struct{
	int value;	
	int kind;
	
}Token;

int main(){
	Token tk;
	tk.value = 2;
	tk.kind = 2;

	//switch(tk.value){	// -----------Bug, tk.value is mistaken as struct type.
	switch(tk.kind){
	case 1:
		printf("%s %d : case 1\n",__FILE__,__LINE__);
		break;
	case 2:
		return !printf("%s %d : case 2\n",__FILE__,__LINE__);
		break;
	case 3:
		printf("%s %d : case 3\n",__FILE__,__LINE__);
		break;
	}
	return 0;
}

