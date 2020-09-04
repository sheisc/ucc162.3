#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "c.h"
////////////////////////////////////////////////////////////////////
static char savedId[MaxIdLen+1];		//保存前一个识别的Id

////////////////////////////////////////////////////////////////////
static void inputValue();
static void outputExprValue();
static int ifStatement();				//if语句
static int assignStatement();			//赋值语句
static int whileStatement();			//while语句
static int printfStatement();			//打印语句
static int scanfStatement();			//输出语句
static int returnStatement();			//return语句
static int compoundStatement();			//复合语句
static void callStatement(char * func);			//函数调用
void callExpr(char * func);
bool isStatementStarting(int tok);		//tok是否为语句开始记号
bool isExpressionStarting(int tok);		//tok是否为表达式开始记号
////////////////////////////////////////////////////////////////////
static int returnStatement(){
	getToken();
	ExprNode node = expression();
	changeConditionToArith(&node);
	decrementTopIfTemp(&node);
	//返回值存放到指定位置，以供上一层的函数来取
	//此时并不改变"变址寄存器"的值
	gen(InsMov,CtrlInfoSize-1,node.address,0);	
	gen(InsRet,0,0,0);
	if(token == SEMICOLON)
		getToken();
	else
		syntaxError("';' missed.");
	return -1;
}
//处理函数调用,不含';'(因为expression和callStatement()都要调用之)
//modified by zcw, 2010.2.25
//之前的callExpr版本存在Bug
void callExpr(char * func){	
	int index = lookup(&globalTable,FUN,func);
	if(index == 0){
		getToken();
		syntaxError("undeclared function name.");
	}
	else{		
		int savedTop = getActiveTop();
		int paramAddr = savedTop+1+CtrlInfoSize;	//存放第一个参数的位置
		setActiveTop(paramAddr+1);
		int count = 0;
		do{					
			getToken();			
			if(isExpressionStarting(token)){
				ExprNode node = expression();
				changeConditionToArith(&node);
				decrementTopIfTemp(&node);
				gen(InsMov,paramAddr,node.address,0);
				paramAddr++;
				setActiveTop(paramAddr+1);
				count ++;
			}		
		}while(token == COMMA);
		if(count !=  globalTable.entries[index].paramCount){
			syntaxError("parameter number not matched.");
		}
		if(token == RP)
			getToken();
		else
			syntaxError("')' missed.");
		//重置当前活动记录的栈顶指针为savedTop
		//如果callExpr函数被partFactor()调用则会在调用callExpr后再调用的
		//setFuncNode()中把activeTop++，
		//（即形如 a = f()+b;的函数调用，要使用函数返回值）
		//这样就把存放返回值的savedTop单元保护起来。
		//而如果callExpr函数被callStatement调用，则说明调用者不必使用返回值，
		//(即形如f();的调用,不必使用函数的返回值）
		//所以savedTop单元可作他用，所以不必在callStatement中做activeTop++。
		setActiveTop(savedTop);
		int newbase = savedTop+1;		 //而savedTop位置用来充当保存函数返回值的临时变量
		int savedCx = csIndex;			
		gen(InsInit,newbase+1,savedCx+4,0);	 //四元式中call指令的下一令指令为返回地址
		gen(InsInit,newbase,newbase,0);		 //上一个活动记录的相对位置
		gen(InsAddBx,newbase,0,0);
		gen(InsCall,globalTable.entries[index].address,0,0);
		//能到这说明已经从子程序返回了，且bx已重设，取回返回值放到临时变量中
		gen(InsMov,savedTop,newbase+2,0);
	}
}
//
void callStatement(char * func){	
	callExpr(func);	
	if(token == SEMICOLON)
		getToken();
	else
		syntaxError("';' missed.");
}
//tok是表达式的开始符号
bool isExpressionStarting(int tok){
	// modified by zcw, 2010.2.25
	//return tok == ID || tok == NUM || tok == LP ;
	return tok == ID || tok == NUM || tok == LP 
		|| (tok == ADDOP && value == SUB) || (tok == NOT);
}
//tok是否为语句开始符号
bool isStatementStarting(int tok){
	//bug removed, 2010.2.26
	//return tok == IF || tok == WHILE || tok == SCANF || tok == RETURN
	//		|| tok == ID || tok == PRINTF || tok == LP || tok == SEMICOLON;
	return tok == IF || tok == WHILE || tok == SCANF || tok == RETURN
			|| tok == ID || tok == PRINTF || tok == LB || tok == SEMICOLON;
}
//复合语句
int compoundStatement(){
	getToken();
	int lastList = -1;
	while(isStatementStarting(token)){
		backpatch(lastList,csIndex);
		lastList = statement();
	}
	if(token == RB)
		getToken();
	else{			
		syntaxError("'}' missed.");
	}
	return lastList;
}
//输出表达式的值
void outputExprValue(){
	ExprNode node = expression();
	decrementTopIfTemp(&node);
	gen(InsOut,node.address,0,0);	
}
//打印语句
int printfStatement(){
	getToken();
	if(token == LP)
		getToken();
	else
		syntaxError("'(' missed");
	outputExprValue();
	while(token == COMMA){
		getToken();
		outputExprValue();		
	}
	if(token == RP)
		getToken();
	else
		syntaxError("')' missed.");
	if(token == SEMICOLON)
		getToken();
	else
		syntaxError("';' missed");
	return -1;
}
//接受输入
void inputValue(){
	if(token == ID){
			SearchingResult result = mixingSearch(VAR,id);
			if(result.index != 0)
				gen(InsIn,result.address,0,0);		
			else
				syntaxError("undeclared identifier");
	}
	else{
		syntaxError("identifier needed here");
	}
	getToken();
}

//输入语句
int scanfStatement(){
	getToken();
	if(token == LP)
		getToken();
	else
		syntaxError("'(' missed");
	inputValue();
	while(token == COMMA){
		getToken();
		inputValue();
	}
	if(token == RP)
		getToken();
	else
		syntaxError("')' missed.");
	if(token == SEMICOLON)
		getToken();
	else 
		syntaxError("';' missed");
	return -1;
}
//赋值语句
int assignStatement(){
	getToken();	
	ExprNode node = expression();
	changeConditionToArith(&node);
	SearchingResult result = mixingSearch(VAR,savedId);
	if(result.index == 0){
		syntaxError("undeclared identifier");
	}
	else{
		gen(InsMov,result.address,node.address,0);
	}
	decrementTopIfTemp(&node);				//释放可能的临时变量
	if(token == SEMICOLON)
		getToken();
	else
		syntaxError("';' missed.");	
	return -1;
}

//处理if语句
static int ifStatement(){
	getToken();
	if(token == LP)
		getToken();
	else
		syntaxError("missing '('.");
	ExprNode node = expression();
	if(token == RP)
		getToken();
	else 
		syntaxError("missing ')'");
	changeArithToCondition(&node);
	backpatch(node.truelist,csIndex);
	int thenList = statement();	
	if(token == ELSE){		
		int savedCx = csIndex;
		gen(InsJmp,0,0,-1);
		backpatch(node.falselist,csIndex);
		getToken();
		int elseList = statement();
		return mergeList(mergeList(savedCx,thenList),elseList);
	}
	else{
		return mergeList(node.falselist,thenList);
	}
}
//处理while语句
int whileStatement(){
	getToken();
	int savedCx = csIndex;
	if(token == LP)
		getToken();
	else
		syntaxError("'(' missed.");
	ExprNode node = expression();
	changeArithToCondition(&node);
	if(token == RP)
		getToken();
	else
		syntaxError("')' missed.");
	backpatch(node.truelist,csIndex);
	int stateList = statement();
	backpatch(stateList,savedCx);
	gen(InsJmp,0,0,savedCx);
	return node.falselist;
	//return mergeList(node.falselist,stateList);
}
/**
* 处理语句
* 返回待回填的链表
*/
int statement(){
	if(token == IF){
		return ifStatement();
	}
	else if(token == WHILE){
		return whileStatement();
	}
	else if(token == RETURN){
		return returnStatement();
	}
	else if(token == ID){
		strcpy(savedId,id);
		getToken();
		if(token == LP){
			callStatement(savedId);
			return -1;
		}else if(token == ASSIGN){
			return assignStatement();
		}else{
			syntaxError("'(' or '=' missed.");
		}
	}
	else if(token == LB){
		return compoundStatement();
	}
	else if(token == SEMICOLON){		
		gen(InsNop,0,0,0);
		getToken();
		return -1;
	}
	else if(token == SCANF){		
		return scanfStatement();
	}
	else if(token == PRINTF){	
		return printfStatement();
	}
	else
		syntaxError("illegal header of a statement");
	return -1;
}

