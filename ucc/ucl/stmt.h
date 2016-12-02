#ifndef __STMT_H_
#define __STMT_H_

#define AST_STATEMENT_COMMON AST_NODE_COMMON

#define AST_LOOP_STATEMENT_COMMON \
    AST_STATEMENT_COMMON          \
    AstExpression expr;           \
    AstStatement stmt;            \
    BBlock loopBB;                \
    BBlock contBB;                \
    BBlock nextBB;

struct astStatement
{
	AST_STATEMENT_COMMON
};
/****************************************
	do-while
	while-do
		astLoopStatement
	for:
		astForStatement
 ****************************************/
typedef struct astLoopStatement
{
	AST_LOOP_STATEMENT_COMMON
} *AstLoopStatement;
//	[expression] ;
typedef struct astExpressionStatement
{
	AST_STATEMENT_COMMON
	AstExpression expr;
} *AstExpressionStatement;
/*********************************************
label-statement:
	identifier:	statement
	case	constant-expression:	statement
	default:		statement
	
However,	in UCC,
	case-statement and default-statement is specially treated.
	see astCaseStatement and astDefaultStatement.

	
 *********************************************/
// identifier:	statement	
typedef struct astLabelStatement
{
	AST_STATEMENT_COMMON
	char *id;
	AstStatement stmt;
	Label label;
} *AstLabelStatement;
// case	constant-expression:	statement
typedef struct astCaseStatement
{
	AST_STATEMENT_COMMON
	AstExpression expr;
	AstStatement  stmt;
	struct astCaseStatement *nextCase;
	BBlock respBB;
} *AstCaseStatement;
// default:		statement
typedef struct astDefaultStatement
{
	AST_STATEMENT_COMMON
	AstStatement stmt;
	BBlock respBB;
} *AstDefaultStatement;
/***********************************
	if(expression)	statement
	if(expression)	statement	else statement
 ************************************/
typedef struct astIfStatement
{
	AST_STATEMENT_COMMON
	AstExpression expr;
	AstStatement  thenStmt;
	AstStatement  elseStmt;
} *AstIfStatement;
/***************************************
	@ncase		number of case
	@minVal	minium value
	@maxVal	maximum value
	@cases		list of case-statement
	@tail		the end of the list of case-statement
	@prev		
 ***************************************/
typedef struct switchBucket
{
	int ncase;
	int	minVal;
	int maxVal;
	AstCaseStatement cases;
	AstCaseStatement *tail;
	struct switchBucket *prev;
} *SwitchBucket;
/**************************************
	@expr		expression
	@stmt		statement
	@cases		list of case-statements
	@defStmt	default-statement
	@buckets	list of switchBucket
	@nbucket	number of nodes in the list of switchBucket
	@nextBB
	@defBB
 **************************************/
// switch(expression) statement
typedef struct astSwitchStatement
{
	AST_STATEMENT_COMMON
	AstExpression expr;
	AstStatement  stmt;
	AstCaseStatement cases;
	AstDefaultStatement defStmt;
	SwitchBucket buckets;
	int nbucket;
	BBlock nextBB;
	BBlock defBB;
} *AstSwitchStatement;
/*************************************************
	@expr	in AST_LOOP_STATEMENT_COMMON
			expression2
	@initExpr	expression1
	@incrExpr	expression3
	@testBB
 *************************************************/
// for([expression1];[expression2];[expression3]) statement
typedef struct astForStatement
{
	AST_LOOP_STATEMENT_COMMON
	AstExpression initExpr;
	AstExpression incrExpr;
	BBlock testBB;
} *AstForStatement;
// goto identifier
typedef struct astGotoStatement
{
	AST_STATEMENT_COMMON
	char *id;
	Label label;
} *AstGotoStatement;
//	break;
typedef struct astBreakStatement
{
	AST_STATEMENT_COMMON
	AstStatement target;
} *AstBreakStatement;
// continue;
typedef struct astContinueStatement
{   
	AST_STATEMENT_COMMON
	AstLoopStatement target;
} *AstContinueStatement;
// return [expression];
typedef struct astReturnStatement
{
	AST_STATEMENT_COMMON
	AstExpression expr;
} *AstReturnStatement;
/***********************************
	@decls		list of declarations
	@stmts		list of statements
	@ilocals	id of local variables  ?
 ***********************************/
// { [declaration-list] 	[statement-list] }
typedef struct astCompoundStatement
{
	AST_STATEMENT_COMMON
	AstNode decls;
	AstNode stmts;
	//	local variables that has initializer-list.
	Vector ilocals;
} *AstCompoundStatement;

#define AsExpr(stmt)   ((AstExpressionStatement)stmt)
#define AsLabel(stmt)  ((AstLabelStatement)stmt)
#define AsCase(stmt)   ((AstCaseStatement)stmt)
#define AsDef(stmt)    ((AstDefaultStatement)stmt)
#define AsIf(stmt)     ((AstIfStatement)stmt)
#define AsSwitch(stmt) ((AstSwitchStatement)stmt)
#define AsLoop(stmt)   ((AstLoopStatement)stmt)
#define AsFor(stmt)    ((AstForStatement)stmt)
#define AsGoto(stmt)   ((AstGotoStatement)stmt)
#define AsCont(stmt)   ((AstContinueStatement)stmt)
#define AsBreak(stmt)  ((AstBreakStatement)stmt)
#define AsRet(stmt)    ((AstReturnStatement)stmt)
#define AsComp(stmt)   ((AstCompoundStatement)stmt)

AstStatement CheckCompoundStatement(AstStatement stmt);

#endif

