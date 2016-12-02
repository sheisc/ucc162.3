#ifndef	LEX_H
#define	LEX_H
//
#define	MAX_ID_LEN	15




//
typedef	union{
	char name[MAX_ID_LEN+1];
	int  numVal;

}Value;

//
typedef enum {
	#define	TOKEN(kind,name)	kind,
	#include "tokens.txt"
	#undef	TOKEN
}TokenKind;
//
typedef struct{
	TokenKind kind;
	Value value;
}Token;


typedef	char (* NEXT_CHAR_FUNC)(void);

#define	MAX_ID_LEN	15
#define	EOF_CH		((char)0xFF)


extern Token curToken;

#define	NEXT_TOKEN	do{curToken = GetToken();}while(0)
Token GetToken(void);
const char * GetTokenName(TokenKind tk);
void InitLexer(NEXT_CHAR_FUNC next);
#endif
