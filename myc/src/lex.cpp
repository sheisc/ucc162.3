#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "c.h"
//////////////////////////////////////////////////////////////////

int token;							//当前记号(的类型)
int value;							//当前记号的值
char id[MaxIdLen+1];					//当前符号串
int lineNumber = 0;						//当前行号
char * types[] = {
	"OR",				//或
	"AND",				//与
	"RELOP",			//关系运算符
	"ADDOP",			//加减	
	"MULOP",			//乘除
	"NOT",				//非
	"LP",				//左括号
	"RP",				//右括号
	"ID",				//标志符
	"NUM",				//数
	"ASSIGN",			//赋值	
	"LB",				//左大括号
	"RB",				//右大括号
	"COMMA",			//逗号
	"SEMICOLON",		//分号
	"UNDEFINED",		//未定义
	"INT",				//int
	"IF",				//if
	"ELSE",				//else
	"WHILE",			//while
	"RETURN",			//return
	"PRINTF",			//printf
	"SCANF",			//scanf
};

//关键字
static char * keywords[KeyWordsCount] = {
	"int",
	"if",
	"else",
	"while",
	"return",
	"printf",
	"scanf",
};
//关键字对应类别
static int keyType[KeyWordsCount] = {
	INT,
	IF,
	ELSE,
	WHILE,
	RETURN,
	PRINTF,
	SCANF,
};

static char lineBuf[1024];					//行缓冲
static int bufIndex = 0;					//缓冲区下标
static int chCount = 0;						//级冲区内字符总数
static char ch = ' ';							//当前字符
		
///////////////////////////////////////////////////////////////////

static void lexError(char * info);
static int isDigit(char ch);
static int isLetter(char ch);
static void nextChar();
static int getKeyWord(char * str);
void getToken();


///////////////////////////////////////////////////////////////////////

/**
* 判断字符串str是否为关键字
* 如果是关键字，则返回关键字的类型编号
* 否则返回-1
* @param char * str  字符串
*/
int getKeyWord(char * str){	
	for(int i = 0; i < KeyWordsCount; i++){
		if(strcmp(str,keywords[i]) == 0){		
			return keyType[i];	
		}
	}
	return UNDEFINED;
}
//错误处理
void lexError(char * info){
	errorCount++;
	printf("line %d: %s\n",lineNumber,info);
	exit(1);
}
/**
* 是否为数字
*/
int isDigit(char ch){
	return ch-'0'>=0 && ch -'9'<=0;
}
/**
*
*/
int isLetter(char ch){
	return (ch-'a'>=0 && ch-'z'<=0) || (ch-'A'>=0 && ch-'Z'<=0);
}
//跳过刚读入行中的注释'//'
static void skipComment(){
	for(int i = 0; i<=chCount-2; i++){
		if(lineBuf[i] == '/' && lineBuf[i+1] == '/'){
			chCount = i+1;
			lineBuf[i] = ' ';
			return ;
		}
	}
}
/** 
*	把下一个字符存到全局变量ch中
*/
void nextChar(){
	char preChar = ' ';
	if(bufIndex == chCount){  //用while循环来处理空行
		if(feof(infile)){
			ch = EOF;		  //设为文件结束标志			
			return;		
		}
		bufIndex =  0;
		chCount = 0;
		char temp = fgetc(infile);
		while(temp != EOF ){			
			if(temp != '\n'){
				lineBuf[chCount] = temp;
				chCount++;
				temp = fgetc(infile);				
			}
			else{
				lineNumber++;
				break;
			}
		}
		lineBuf[chCount++] = ' ';   //把回车换行符视为分隔符空格
		skipComment();
	}
	ch = lineBuf[bufIndex];	
	bufIndex++;
}
/**
*	取下一个记号,并把相关信息存到token  value  id中 
* 	
*/
void getToken(){
	
	//跳过空白
	while(ch == ' ' || ch == '\t' || ch=='\r') 
		nextChar();
	
	if( ch == '+'){
		token = ADDOP;
		value = ADD;
		nextChar();
	}
	else if(ch == '-'){
		token = ADDOP;
		value = SUB;
		nextChar();
	}
	else if(ch == '%'){
		token = MULOP;
		value = MOD;
		nextChar();
	}
	else if(ch == '*'){
		token = MULOP;
		value = MUL;
		nextChar();
	}
	else if(ch == '/'){
		token = MULOP;
		value = DIV;
		nextChar();
	}
	else if(ch == '('){
		token = LP;
		nextChar();
	}
	else if(ch == ')'){
		token = RP;
		nextChar();
	}
	else if(ch == '='){
		nextChar();
		if( ch == '='){
			token = RELOP;
			value = EQ;
			nextChar();
		}
		else
			token = ASSIGN;
	}
	else if(ch == '<'){
		token = RELOP;
		nextChar();
		if( ch == '='){				
			value = LE;
			nextChar();
		}
		else
			value = LT;
	}
	else if(ch == '>'){
		token = RELOP;
		nextChar();
		if(ch == '='){
			value = GE;
			nextChar();
		}
		else
			value = GT;
	}
	else if(ch == '&'){
		nextChar();
		if(ch == '&'){
			token = AND;
			nextChar();
		}
		else
			lexError("& is missing.");		
	}
	else if(ch == '|'){
		nextChar();
		if(ch == '|'){
			token = OR;
			nextChar();
		}
		else
			lexError("| is missing.");			
	}
	else if(ch == '!'){
		nextChar();
		if( ch == '='){
			token = RELOP;
			value = NE;
			nextChar();
		}
		else 
			token = NOT;
	}
	else if(isLetter(ch)){		
		int index = 0;
		id[index++] = ch;
		nextChar();
		while(isLetter(ch) || isDigit(ch)){
			if(index < MaxIdLen){
				id[index++] = ch;
				nextChar();
			}
			else{
				lexError("identifier is too long.");
				break;
			}
		}
		id[index] = '\0';
		token = getKeyWord(id);
		if(token == UNDEFINED)
			token = ID;
	}
	else if(isDigit(ch)){
		token = NUM;
		int num = 0;
		num = ch - '0';
		nextChar();
		while( isDigit(ch)){
			num = num * 10 + (ch - '0');
			nextChar();
		}
		value = num;
	}
	else if(ch == '{'){
		token = LB;
		nextChar();
	}
	else if(ch == '}'){
		token = RB;
		nextChar();
	}
	else if(ch == ','){
		token = COMMA;
		nextChar();
	}
	else if(ch == ';'){
		token = SEMICOLON;
		nextChar();
	}	
	else if(ch == EOF){
		token = EOF;
#ifdef ZCW_DEBUG
		printf("end of file.\n");		
#endif
	}
	else{
		lexError("Fatal error: unknown character.");
		exit(0);
	}
	/*
	if(token != EOF)
		printf("%s %s %d \n",types[token],id,value);	
	*/
	
}
