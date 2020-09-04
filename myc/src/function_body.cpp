#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "c.h"

/**
* 处理函数体
*/
void funcBody(){
	int savedCx = csIndex;
	if(token == LB)
		getToken();
	else
		syntaxError("'{' missed.");
	int lastList = -1;
	while(isStatementStarting(token) || token == INT){
		backpatch(lastList,csIndex);
		if(token == INT){
			getToken();			
			varDeclare();
			lastList = -1;		//声明语句不用回填
		}
		else {
			lastList = statement();
		}		
	}
	backpatch(lastList,csIndex);	//回填函数体中最后一条语句	
	if(token == RB)
		getToken();
	else
		syntaxError("'}' missed.");	
	gen(InsRet,0,0,0);					//产生一条语句确保函数能返回
	
}
