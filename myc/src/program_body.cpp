#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "c.h"
////////////////////////////////////////////////////////////////

static int  unsetJumpCx = 1;			//要设为jmp，但不知jump到何处的指令编号
static char savedId[MaxIdLen+1];		//缓存标志符
static bool globalState = true;			//是否为全局状态

////////////////////////////////////////////////////////////////
static void setJump(int cx, int target);
static void	 semicolonPart();
static void  lpPart();
static void	 commaPart();
static void assignPart();
bool isGlobalState();
static void jumpToMain();			//跳转到main()函数
static void allocGlobalMem();		//分配全局变量区
static void intial();				//初始化


////////////////////////////////////////////////////////////////
static void initial(){
	jumpToMain();
	allocGlobalMem();
	int bx = getGlobalMemSize()+1;
	ds[bx] = -1;	    	//上一个活动记录的地址
	ds[bx+1] = HaltingPC;   //返回地址，即返回后要执行的第一条指令的地址
					    	//main()函数的返回地址为-1，表示程序结束
}
//通过设置"变址寄存器Bx"的值来分配全局变量区
//即数据区的ds[1]到ds[getGlobalMemSize()]的区域为全局变量区
void allocGlobalMem(){
	cs[0].optr = InsSetBx;
	cs[0].arg1 = getGlobalMemSize()+1;
	cs[0].arg2 = 0;
	cs[0].result = 0;
}

//跳到main函数
static void jumpToMain(){
	int index = lookup(&globalTable,FUN,"main");
	if(index == 0){
		syntaxError("Fatal error: main( ) needed\n");
	}
	else{
		setJump(unsetJumpCx,globalTable.entries[index].address);
	}
}
//设置第cx条指令跳转到target
static void setJump(int cx, int target){
	cs[cx].optr = InsJmp;
	cs[cx].arg1 = 0;
	cs[cx].arg2 = 0;
	cs[cx].result = target;
}
//处理id后跟左括号的情况
static void lpPart(){
	localTable.index = 0;				//重置符号表
	globalState = false;
	int savedCx = csIndex;	
	getToken();
	int index = lookup(&globalTable,FUN,savedId);
	if(index == 0)
		index = enter(&globalTable,FUN,savedId);
	else
		syntaxError("redeclared function name.");					
	globalTable.entries[index].paramCount = paramDeclare();		
	if(token == RP){
		getToken();				
	}
	else{
		syntaxError("missing ')'.");
	}
	globalTable.entries[index].address = csIndex;			
	funcBody();
}
//处理id后跟分号的情况
static void	 semicolonPart(){
	setJump(unsetJumpCx,csIndex);
	int index = lookup(&globalTable,VAR,savedId);
	if(index == 0){		
		index = enter(&globalTable,VAR,savedId);				
		//全局变量默认初始化为0
		gen(InsInit,globalTable.entries[index].address,0,0);
		
	}
	else
		syntaxError("redeclare global identifier");
	unsetJumpCx = csIndex;
	gen(InsJmp,0,0,-1);	
	getToken();
}
//处理id后跟逗号的情况
static void commaPart(){
	setJump(unsetJumpCx,csIndex);
	int index = lookup(&globalTable,VAR,savedId);
	if(index == 0){
		index = enter(&globalTable,VAR,savedId);				
		//全局变量默认初始化为0
		gen(InsInit,globalTable.entries[index].address,0,0);
	}
	else
		syntaxError("redeclare global identifier");			
	getToken();
	varDeclare();
	unsetJumpCx = csIndex;
	gen(InsJmp,0,0,-1);
	
}
//处理id后跟赋值号的情况
static void assignPart(){
	setJump(unsetJumpCx,csIndex);
	int index = lookup(&globalTable,VAR,savedId);
	if(index == 0)
		index = enter(&globalTable,VAR,savedId);
	else
		syntaxError("redeclare global identifier");
	
	getToken();
	ExprNode node = expression();
	changeConditionToArith(&node);			
	decrementTopIfTemp(&node);
	
	gen(InsMov,globalTable.entries[index].address,node.address,0);			
	if(token == SEMICOLON){
		getToken();
	}
	else if(token == COMMA){
		getToken();
		varDeclare();
	}
	else{
		syntaxError("missing ',' or ';'.");
	}
	unsetJumpCx = csIndex;
	gen(InsJmp,0,0,-1);
	
}
//
bool isGlobalState(){
	return globalState;
}

/**
* 处理程序体
*/
void programBody(){		
	while(token == INT){
		
		setActiveTop(CtrlInfoSize);		//活动记录栈顶指针复位
		globalState = true;		//置为全局状态
		getToken();	
		strcpy(savedId,id);
		if(token == ID){
			getToken();	
		}
		else{
			syntaxError("identifier is needed here");
		}
		switch(token){
			case LP:
				lpPart();
				break;
			case SEMICOLON:
				semicolonPart();
				break;
			case COMMA:
				commaPart();
				break;
			case ASSIGN:
				assignPart();
				break;
			default:
				syntaxError("identifier followed by some illegal character");
		}
	}
	initial();

}
