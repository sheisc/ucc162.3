#ifndef	EXPR_H
#define	EXPR_H
#include "lex.h"

#define MAX_ARG_CNT	10
#define CPU_WORD_SIZE_IN_BYTES   4

typedef struct astNode{
	TokenKind op;
	Value value;
	// assembly  name  for this node
	char accessName[MAX_ID_LEN+1];
	struct astNode * kids[2];
	int offset;
	// for function call
	struct astNode *args[MAX_ARG_CNT];
	int arg_cnt;
} * AstNodePtr;




#define		Expect(tk)		DoExpect(__LINE__,__FILE__,tk)
//void Expect(TokenKind tk);
void DoExpect(int line, char * fileName, TokenKind  tk);
AstNodePtr Expression(void);
void VisitArithmeticNode(AstNodePtr pNode);
void EmitArithmeticNode(AstNodePtr pNode);
char  *  GetAccessName(AstNodePtr pNode);
int GetFrameSize(void);
void ResetFrameSize(void);
int GetFrameOffset(void);
AstNodePtr CreateAstNode(TokenKind tk,Value * pVal,AstNodePtr left,AstNodePtr right);
AstNodePtr FunctionCallExpression(Token savedToken);

#endif
