#ifndef	DECL_H
#define	DECL_H
#include "lex.h"
#include "expr.h"
AstNodePtr Declaration(void);
void VisitDeclarationNode(AstNodePtr pNode);
#endif
