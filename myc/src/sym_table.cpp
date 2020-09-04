#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "c.h"
//////////////////////////////////////////////////////////////////
static int activeTop = CtrlInfoSize;	//活动记录的栈顶指针
static int globalTop = 1;	//全局数据区的游标
static SymbolEntry globalEntries[MaxGSize+1];
static SymbolEntry localEntries[MaxLSize+1];
SymbolTable globalTable = {globalEntries,0};	//全局符号表
SymbolTable localTable = {localEntries,0};	//局部符号表

//////////////////////////////////////////////////////////////////
void setActiveTop(int top);
int newtemp();
void freeTemp();
int getGlobalMemSize();
int getActiveTop();			//调试
void exitIfFull(SymbolTable * table,int idType);
static bool isGlobalTable(SymbolTable * table);
SearchingResult mixingSearch(int idType,char * lexeme);
//////////////////////////////////////////////////////////////////
int getGlobalMemSize(){
	return (globalTop-1);
}
//重置activeTop指针
void setActiveTop(int top){
	activeTop = top;
}
//判断是否为全局符号表
static bool isGlobalTable(SymbolTable * table){
	if( table == &globalTable)
		return true;
	else 
		return false;
}
// 获得一个临时变量
int newtemp(){
	int temp = activeTop;
	activeTop++;
	return temp;	
	
}
int getActiveTop(){
	return activeTop;
}
//释放一个临时变量
void freeTemp(){
	activeTop--;	
}
//若符号表已满，则退出
static void exitIfFull(SymbolTable * table){
	if(isGlobalTable(table)){
		if(table->index >= MaxGSize){
			printf("global table is full.\n");
			exit(1);
		}
	}
	else if(table->index >= MaxLSize){
		printf("local table is full.\n");
		exit(1);
	}	
}
//在符号表表尾追加一项,本函数不判断表中是否已经存在名为id的标志符
int enter(SymbolTable * table,int idType,char * lexeme ){	
	exitIfFull(table);
	table->index++;
	strcpy(table->entries[table->index].lexeme,lexeme);	
	table->entries[table->index].type = idType;
	//局部变量所占空间在未分配给局部变量时，可能被充当临时
	//变量使用，但一旦分配之后，就不再被临时变量所用	
	if(!isGlobalTable(table))
		table->entries[table->index].address = newtemp();		
	else{
		//用负数表示全局变量的地址,函数名的地址要翻译完函数体后再填
		if(idType == VAR){
			table->entries[table->index].address = -globalTop;
			globalTop++;
		}
	}
	return table->index;
}
//判断lexeme是否在符号表中，如果存在，则返回表项位置
//如果不存在，则返回0
int lookup(SymbolTable * table,int idType,char * lexeme){
	//第0项作为哨兵
	strcpy(table->entries[0].lexeme,lexeme);
	// added by zcw, 2010.2.24
	table->entries[0].type = idType;
	//查找是否有匹配的
	int i = table->index;
	while(strcmp(table->entries[i].lexeme,lexeme)!=0
		|| table->entries[i].type != idType){
			i--;
	}
	return i;
	// commented by zcw  , 2010.2.24
	//for(int i = table->index; i >= 0; i--){
	//	if(strcmp(table->entries[i].lexeme,lexeme)==0
	//		&& table->entries[i].type == idType){
	//		return i;
	//	}
	//}
	//return 0;
}
//混合式查找，先找局部符号表，再找全局符号表
SearchingResult mixingSearch(int idType,char * lexeme){
	SearchingResult result;
	result.index = lookup(&localTable,idType,lexeme);
	result.tableType = LOCAL;
	if(result.index != 0){
		result.address = localTable.entries[result.index].address;		
	}
	else{
		result.index = lookup(&globalTable,idType,lexeme);
		result.tableType = GLOBAL;
		if(result.index != 0)
			result.address = globalTable.entries[result.index].address;
	}	
	return  result;
}

