#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "lex.h"
#include "error.h"

typedef struct{
	TokenKind kind;
	char * name;
}KeywordInfo;

static char defaultNextChar(void);

static char curChar = ' ';
static NEXT_CHAR_FUNC NextChar = defaultNextChar;
static KeywordInfo keywords[] = {
	{TK_INT,"int"},
	{TK_IF,"if"},
	{TK_ELSE,"else"},
	{TK_WHILE,"while"},
};
static char * tokenNames[] = {
	#define	TOKEN(kind,name)	name,
	#include "tokens.txt"
	#undef	TOKEN
};
Token curToken;

////////////////////////////////////////////////////////////////////////
static TokenKind GetKeywordKind(char * id){
	int i = 0; 
	for(i = 0; i < sizeof(keywords)/sizeof(keywords[0]); i++){
		if(strcmp(id,keywords[i].name) == 0){
			return keywords[i].kind;
		}
	}
	return TK_ID;
}
static int IsWhiteSpace(char ch){
	return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r';
}
static char defaultNextChar(void){
	return EOF_CH;
}
//
static TokenKind GetTokenKindOfChar(char ch){
	int i = 0;
	for(i = 0; i < sizeof(tokenNames)/sizeof(tokenNames[0]); i++){
		if((strlen(tokenNames[i]) == 1) && (tokenNames[i][0] == ch)){
			return i;
		}
	}
	return TK_NA;
}
const char * GetTokenName(TokenKind tk){
	return tokenNames[tk];
}
//
Token GetToken(void){
	Token token;
	int len = 0;
	memset(&token,0,sizeof(token));
	// skip white space
	while(IsWhiteSpace(curChar)){
		curChar = NextChar();
	}
TryAgain:
	if(curChar == EOF_CH){
		token.kind = TK_EOF;
	}else if(isalpha(curChar)){//id or keyword
		len = 0;
		do{				
			token.value.name[len] = curChar;
			curChar = NextChar();
			len++;
		}while(isalnum(curChar) && len < MAX_ID_LEN);
		token.kind = GetKeywordKind(token.value.name);
	}else if(isdigit(curChar)){//number
		int numVal = 0;
		token.kind = TK_NUM;
		do{
			numVal = numVal*10 + (curChar-'0');
			curChar = NextChar();
		}while(isdigit(curChar));
		token.value.numVal = numVal;
	}else{
		token.kind = GetTokenKindOfChar(curChar);
		if(token.kind != TK_NA){// '+','-','*','/',...
			token.value.name[0] = curChar;
			curChar = NextChar();
		}else{
			Error("illegal char \'%x\' .\n",curChar);
			curChar = NextChar();
			goto TryAgain;
		}
	}
	return token;
}

void InitLexer(NEXT_CHAR_FUNC next){
	if(next){
		NextChar = next;
	}
}
