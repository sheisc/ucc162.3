#ifndef __LEX_H_
#define __LEX_H_
/*************************************
 token.h:
	 TOKEN(TK_TYPEDEF,	 "typedef")
	 TOKEN(TK_CONST,	 "const")
Token Type	 
 *************************************/
enum token
{
	TK_BEGIN,
#define TOKEN(k, s) k,
#include "token.h"
#undef  TOKEN

};
// Token Value
union value
{
	int i[2];
	float f;
	double d;
	void *p;
};

#define IsDigit(c)         (c >= '0' && c <= '9')
#define IsOctDigit(c)      (c >= '0' && c <= '7')
#define IsHexDigit(c)      (IsDigit(c) || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))
#define IsLetter(c)        ((c >= 'a' && c <= 'z') || (c == '_') || (c >= 'A' && c <= 'Z'))
#define IsLetterOrDigit(c) (IsLetter(c) || IsDigit(c))
#define ToUpper(c)		   (c & ~0x20)
#define HIGH_4BIT(v)       ((v) >> (8 * sizeof(int) - 4) & 0x0f)
#define HIGH_3BIT(v)       ((v) >> (8 * sizeof(int) - 3) & 0x07)
#define HIGH_1BIT(v)       ((v) >> (8 * sizeof(int) - 1) & 0x01)

void SetupLexer(void);
void BeginPeekToken(void);
void EndPeekToken(void);
int  GetNextToken(void);

extern union value  TokenValue;
extern struct coord TokenCoord;
extern struct coord PrevCoord;
extern char* TokenStrings[];

#endif

