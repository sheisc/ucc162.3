#ifndef	STMT_H
#define	STMT_H
#include "lex.h"
#include "expr.h"

typedef struct astStmtNode{
	TokenKind op;
	Value value;
	struct astNode * kids[2];
	/////////////////////////////
	struct astNode * expr;
	struct astStmtNode * thenStmt;
	struct astStmtNode * elseStmt;
	struct astStmtNode * next;
} * AstStmtNodePtr;
AstStmtNodePtr CompoundStatement(void);
AstStmtNodePtr Statement(void);
void VisitStatementNode(AstStmtNodePtr stmt);
#endif

