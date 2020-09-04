#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "c.h"
///////////////////////////////////////////////////////////////////
void processOneVar();

///////////////////////////////////////////////////////////////////

void processOneVar(){	
	if(token == ID){		
		SymbolTable * table;
		if(isGlobalState()){
			table = &globalTable;
		}
		else{
			table = &localTable;
		}
		int index = lookup(table,VAR,id);		
		if(index == 0){
			index =	enter(table,VAR,id);
		}
		else{
			syntaxError("redeclared identifier.");
		}
		getToken();
		if(token == ASSIGN){		
			getToken();
			ExprNode node = expression();
			changeConditionToArith(&node);			
			decrementTopIfTemp(&node);
			gen(InsMov,table->entries[index].address,node.address,0);
		}
	}
}

void varDeclare(){
	if(token == ID){
		processOneVar();
		while(token == COMMA){
			getToken();
			processOneVar();
		}
		if(token == SEMICOLON){
			getToken();
		}
		else{
			syntaxError("';' missed.");
		}		
	}
}

