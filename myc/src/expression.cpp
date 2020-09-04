#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "c.h"
/////////////////////////////////////////////////////////////

static char savedId[MaxIdLen+1];
//////////////////////////////////////////////////////////////
static void setFuncNode(ExprNode * node);
static void setTempNode(ExprNode * node,int val);
static void setIdNode(ExprNode * node);
static void setGlobalNode(ExprNode * node);
static ExprNode andTerm();			//与项
static ExprNode relationTerm();	//关系项
static ExprNode addTerm();			//和项
static ExprNode mulTerm();			//乘项
static ExprNode factor();			//因子

bool isCondition(ExprNode * node);
void decrementTopIfTemp(ExprNode * node);
void setExprNode(ExprNode * node, int tlist,int flist,int addr, bool isTemp);
int mergeList(int list1, int list2);  //合并链表
void backpatch(int list, int cx);	  //回填链表
void changeConditionToArith(ExprNode * node);
void changeArithToCondition(ExprNode * node);
ExprNode expression();		//表达式
///////////////////////////////////////////////////////////////



/**
* 表达式
*/
ExprNode expression(){	
	ExprNode left = andTerm();	
	while(token == OR){			
		changeArithToCondition(&left);
		getToken();
		backpatch(left.falselist,csIndex);	
		ExprNode right = andTerm();			
		changeArithToCondition(&right);			
		left.truelist = mergeList(left.truelist,right.truelist);
		left.falselist = right.falselist;
	}
	return left;
}
/**
* 与项
*/
ExprNode andTerm(){
	ExprNode left = relationTerm();		
	while(token == AND){
		getToken();
		changeArithToCondition(&left);
		backpatch(left.truelist,csIndex);
		ExprNode right = relationTerm();		
		changeArithToCondition(&right);			
		left.truelist = right.truelist;
		left.falselist = mergeList(left.falselist,right.falselist);
	}
	return left;
}

/**
* 关系项
*/
ExprNode relationTerm(){
	ExprNode left,right;
	left = addTerm();		
	if(token == RELOP){	
		changeConditionToArith(&left);
		int temp = value;			//保存关系运算符类型
		int relopOp;
		getToken();
		right = addTerm();		
		changeConditionToArith(&right);
		switch(temp){
		case GT:			
			relopOp = InsJgt;
			break;
		case GE:			
			relopOp = InsJge;
			break;
		case EQ:
			relopOp = InsJeq;
			break;
		case LT:			
			relopOp = InsJlt;
			break;
		case LE:			
			relopOp = InsJle;
			break;
		case NE:			
			relopOp = InsJne;
			break;
		}		
		//		
		decrementTopIfTemp(&left);
		decrementTopIfTemp(&right);		
		left.truelist = csIndex;
		left.falselist = csIndex+1;
		gen(relopOp,left.address,right.address,-1);
		gen(InsJmp,0,0,-1);		
	}			
	return left;
	
}
/**
 * 处理"和项"，
 * 和项由(乘项1,ADDOP,乘项2)构成
 */
ExprNode addTerm(){
	ExprNode left = mulTerm();
	while(token == ADDOP){		
		changeConditionToArith(&left);
		int temp = value, instrType;
		getToken();
		ExprNode right= mulTerm();		
		if(temp == ADD)
			instrType = InsAdd;
		else if(temp == SUB)
			instrType = InsSub;
		changeConditionToArith(&right);
		decrementTopIfTemp(&left);
		decrementTopIfTemp(&right);
		int dest = newtemp();
		gen(instrType,left.address,right.address,dest);
		setExprNode(&left,-1,-1,dest,true);		
	}
	return left;
}
/**
 * 处理"乘项"，
 * 乘项由(因子1,MULOP,因子2)构成
 */
ExprNode mulTerm(){
	ExprNode left,right;
	left = factor();
	while(token == MULOP){
		changeConditionToArith(&left);
		int savedValue = value, instrType;
		getToken();
		right = factor();
		changeConditionToArith(&right);
		if(savedValue == MUL)
			instrType = InsMul;
		else if(savedValue == DIV)
			instrType = InsDiv;
		else if(savedValue == MOD)
			instrType = InsMod;
		decrementTopIfTemp(&left);
		decrementTopIfTemp(&right);
		int dest = newtemp();
		gen(instrType,left.address,right.address,dest);
		setExprNode(&left,-1,-1,dest,true);
	}
	return left;
}
/**
* 设置ExprNode的值
*/
void setExprNode(ExprNode * node, int tlist,int flist,int addr,bool isTemp){
	node->truelist = tlist;
	node->falselist = flist;
	node->address = addr;	
	node->isTemp = isTemp;
}
//设置存放函数值的临时变量对应的表达式结点
static void setFuncNode(ExprNode * node){
	int temp = newtemp();
	node->address = temp;
	node->isTemp = true;
	node->truelist = -1;
	node->falselist = -1;
}
//设置存放值val的临时变量对应的表达式结点
static void setTempNode(ExprNode * node,int val){
	int temp = newtemp();
	node->address = temp;
	gen(InsInit,node->address,val,0);
	node->isTemp = true;
	node->truelist = -1;
	node->falselist = -1;
}
//设置标志符对应的表达式结点
static void setIdNode(ExprNode * node){	
	if(!isGlobalState()){
		SearchingResult result = mixingSearch(VAR,savedId);
		if(result.index == 0){
			syntaxError("identifier  undeclared.");
			setTempNode(node,0);		//错误处理，视为值为0的临时变量
		}
		else{
			node->isTemp = false;
			node->address = result.address;
			node->truelist = -1;
			node->falselist = -1;
		}
	}
	else{
		int index = lookup(&globalTable,VAR,savedId);
		if(index == 0){
			syntaxError("identifier  undeclared.");
			setTempNode(node,0);		//错误处理，视为值为0的临时变量
		}
		else{
			node->isTemp = false;
			node->address = globalTable.entries[index].address;
			node->truelist = -1;
			node->falselist = -1;
		}
	}
}
// funtion 'partFactor'  added by zcw, 2010.2.25
ExprNode partFactor(){
	ExprNode result;	
	memset(&result,0,sizeof(ExprNode));
	if(token == ID){	
		strcpy(savedId,id);
		getToken();
		if(token == LP){
			callExpr(savedId);
			setFuncNode(&result);
		}else{
			setIdNode(&result);
		}			
	}else if(token == NUM){			
		setTempNode(&result,value);		
		getToken();
	}else{//(token == LP){
		getToken();
		result = expression();
		if(token == RP)
			getToken();
		else
			syntaxError("missing ')'");
	}
	return result;
}
/**
 * 处理"因子"，
 * 因子由标识符、数、(NOT 因子)或(表达式）构成
 */
ExprNode factor(){
	ExprNode result;	
	//modified by zcw because grammar graph is changed, 2010.2.25
	if(token == ID || token == NUM || token == LP){	
		result = partFactor();			
	}else if(token == NOT){ 
		getToken();
		result = partFactor();
		changeArithToCondition(&result);
		int temp = result.falselist;
		result.falselist = result.truelist;
		result.truelist = temp;	
	}else if(token == ADDOP && value == SUB){
		getToken();
		result = partFactor();
		changeConditionToArith(&result);
		decrementTopIfTemp(&result);
		int temp = newtemp();	
		gen(InsUminus,result.address,0,temp);
		result.address = temp;
		result.truelist = true;
		result.falselist = -1;
		result.truelist = -1;		
	}	
	else
		syntaxError("Indentifier ,number, or '(' needed");
	return result;
}


/**
* 合并两个有待回填的链表,
* 返回存放作为新链表首项的四元式编号
* @param int list1  链表1表首对应的四元式编号
* @param int list2  链表2表首对应的四元式编号 
*/
int mergeList(int list1, int list2){
	if(list1 != -1){
		int cur = list1;
		//找到result为-1的那条指令，即链表的表尾
		while( cs[cur].result != -1){
			cur = cs[cur].result;
		}
		cs[cur].result = list2;
		return list1;
	}
	else{
		return list2;
	}
}

/**
* 以编号cx的四元式作为出口，回填链表 
* @param int list  待回填的链表表首对应的四元式编号
* @param int cx    出口对应的四元式编号
*/
void backpatch(int list, int cx){
	
	int next;
	//如果list为四元式的编号
	while(list != -1){		
		next = cs[list].result;
		cs[list].result = cx;
		list = next;
	}
}
//如果是临时变量，则使用后，可使activeTop--;
void decrementTopIfTemp(ExprNode * node){
	if(node->isTemp) 
		freeTemp();
}
/**
 * 判断是否为布尔量
 */
bool isCondition(ExprNode * node){
	return (node->truelist != -1) || (node->falselist != -1);
}
/**
 * 把条件表达式当作算术表达式来翻译
 * @param ExprNode * node  表达式结点
 */
void changeConditionToArith(ExprNode * node){	
	if(isCondition(node)){
		//申请一个临时变量来存放0或1
		node->address = newtemp();
		node->isTemp = true;	
		int savedIndex = csIndex;
		gen(InsInit,node->address,1,0);
		gen(InsJmp,0,0,savedIndex+3);
		gen(InsInit,node->address,0,0);		
		backpatch(node->truelist,savedIndex);
		backpatch(node->falselist,savedIndex+2);
		//转变为算术量
		node->truelist = -1;
		node->falselist = -1;
	}
}


/**
 * 把算述表达式当作条件表达式来翻译
 * @param ExprNode * node  表达式结点
 */
void changeArithToCondition(ExprNode * node){
	if(!isCondition(node)){			
		node->truelist = csIndex;
		node->falselist = csIndex+1;		
		gen(InsJtrue,node->address,0,-1);	
		gen(InsJmp,0,0,-1);	
		decrementTopIfTemp(node);
	}
}
