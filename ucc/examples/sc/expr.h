#ifndef	EXPR_H
#define	EXPR_H
#include "lex.h"

typedef struct astNode{
	TokenKind op;
	Value value;
	struct astNode * kids[2];
} * AstNodePtr;





void Expect(TokenKind tk);
AstNodePtr Expression(void);
void VisitArithmeticNode(AstNodePtr pNode);
AstNodePtr CreateAstNode(TokenKind tk,Value * pVal,AstNodePtr left,AstNodePtr right);
#endif
