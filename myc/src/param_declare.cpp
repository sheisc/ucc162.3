#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "c.h"
///////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
//处理一个声明
static void processOneDeclare(){
	if(token == INT){
		getToken();
		if(token == ID){
			int index = lookup(&localTable,VAR,id);
			if(index == 0)
				index = enter(&localTable,VAR,id);
			else
				syntaxError("reclared paramname.");			
			getToken();
		}
		else{
			syntaxError("paramname missed.");
		}	
	}
	
}
/**
* 函数参数声明,返回参数的个数,可能根本就没有参数
*/
int paramDeclare(){	
	int count = 0;
	if(token == INT){
		count++;
		processOneDeclare();
		while(token == COMMA){
			getToken();
			processOneDeclare();
			count ++;
		}
	}
	return count;

}

