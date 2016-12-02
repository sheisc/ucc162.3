#include "ucl.h"
#include "grammer.h"
#include "ast.h"
#include "stmt.h"

static int FIRST_Statement[] = { FIRST_STATEMENT, 0};

static AstStatement ParseStatement(void);

/**
 *  expression-statement:
 *		[expression] ;
 */
static AstStatement ParseExpressionStatement(void)
{
	AstExpressionStatement exprStmt;

	CREATE_AST_NODE(exprStmt, ExpressionStatement);

	if (CurrentToken != TK_SEMICOLON)
	{
		exprStmt->expr = ParseExpression();
	}
	Expect(TK_SEMICOLON);

	return (AstStatement)exprStmt;
}

/**
 *  label-statement:
 *		ID : statement
 */
static AstStatement ParseLabelStatement(void)
{
	AstLabelStatement labelStmt;
	int t;

	BeginPeekToken();
	t = GetNextToken();
	if (t == TK_COLON)
	{
		EndPeekToken();
		CREATE_AST_NODE(labelStmt, LabelStatement);
		labelStmt->id = TokenValue.p;

		NEXT_TOKEN;
		NEXT_TOKEN;
		labelStmt->stmt = ParseStatement();

		return (AstStatement)labelStmt;
	}
	else
	{
		EndPeekToken();
		return ParseExpressionStatement();
	}
}

/**
 *  case-statement:
 *		case constant-expression : statement
 */
static AstStatement ParseCaseStatement(void)
{
	AstCaseStatement caseStmt;

	CREATE_AST_NODE(caseStmt, CaseStatement);

	NEXT_TOKEN;
	caseStmt->expr = ParseConstantExpression();
	Expect(TK_COLON);
	caseStmt->stmt = ParseStatement();

	return (AstStatement)caseStmt;
}

/**
 *  default-statement:
 *		default : statement
 */
static AstStatement ParseDefaultStatement(void)
{
	AstDefaultStatement defStmt;

	CREATE_AST_NODE(defStmt, DefaultStatement);

	NEXT_TOKEN;
	Expect(TK_COLON);
	defStmt->stmt = ParseStatement();

	return (AstStatement)defStmt;
}

/**
 *  if-statement:
 *		if ( expression ) statement
 *		if ( epxression ) statement else statement
 */
static AstStatement ParseIfStatement(void)
{
	AstIfStatement ifStmt;

	CREATE_AST_NODE(ifStmt, IfStatement);

	NEXT_TOKEN;
	Expect(TK_LPAREN);
	ifStmt->expr = ParseExpression();
	Expect(TK_RPAREN);
	ifStmt->thenStmt = ParseStatement();
	if (CurrentToken == TK_ELSE)
	{
		NEXT_TOKEN;
		ifStmt->elseStmt = ParseStatement();
	}

	return (AstStatement)ifStmt;
}

/**
 *  switch-statement:
 *		switch ( expression ) statement
 */
static AstStatement ParseSwitchStatement(void)
{
	AstSwitchStatement swtchStmt;

	CREATE_AST_NODE(swtchStmt, SwitchStatement);

	NEXT_TOKEN;
	Expect(TK_LPAREN);
	swtchStmt->expr = ParseExpression();
	Expect(TK_RPAREN);
	swtchStmt->stmt = ParseStatement();

	return (AstStatement)swtchStmt;
}

/**
 *  while-statement:
 *		while ( expression ) statement
 */
static AstStatement ParseWhileStatement(void)
{
	AstLoopStatement whileStmt;

	CREATE_AST_NODE(whileStmt, WhileStatement);

	NEXT_TOKEN;
	Expect(TK_LPAREN);
	whileStmt->expr = ParseExpression();
	Expect(TK_RPAREN);
	whileStmt->stmt = ParseStatement();

	return (AstStatement)whileStmt;
}

/**
 *  do-statement:
 *		do statement while ( expression ) ;
 */
static AstStatement ParseDoStatement()
{
	AstLoopStatement doStmt;

	CREATE_AST_NODE(doStmt, DoStatement);

	NEXT_TOKEN;
	doStmt->stmt = ParseStatement();
	Expect(TK_WHILE);
	Expect(TK_LPAREN);
	doStmt->expr= ParseExpression();
	Expect(TK_RPAREN);
	Expect(TK_SEMICOLON);

	return (AstStatement)doStmt;
}

/**
 *  for-statement:
 *		for ( [expression] ; [expression] ; [expression] ) statement
 */
static AstStatement ParseForStatement()
{
	AstForStatement forStmt;

	CREATE_AST_NODE(forStmt, ForStatement);

	NEXT_TOKEN;
	Expect(TK_LPAREN);
	if (CurrentToken != TK_SEMICOLON)
	{
		forStmt->initExpr = ParseExpression();
	}
	Expect(TK_SEMICOLON);
	if (CurrentToken != TK_SEMICOLON)
	{
		forStmt->expr = ParseExpression();
	}
	Expect(TK_SEMICOLON);
	if (CurrentToken != TK_RPAREN)
	{
		forStmt->incrExpr = ParseExpression();
	}
	Expect(TK_RPAREN);
	forStmt->stmt = ParseStatement();

	return (AstStatement)forStmt;
}

/**
 *  goto-statement:
 *		goto ID ;
 */
static AstStatement ParseGotoStatement(void)
{
	AstGotoStatement gotoStmt;

	CREATE_AST_NODE(gotoStmt, GotoStatement);

	NEXT_TOKEN;
	if (CurrentToken == TK_ID)
	{
		gotoStmt->id = TokenValue.p;
		NEXT_TOKEN;
		Expect(TK_SEMICOLON);
	}
	else
	{
		Error(&TokenCoord, "Expect identifier");
		if (CurrentToken == TK_SEMICOLON)
			NEXT_TOKEN;
	}

	return (AstStatement)gotoStmt;
}

/**
 *  break-statement:
 *		break ;
 */
static AstStatement ParseBreakStatement(void)
{
	AstBreakStatement brkStmt;

	CREATE_AST_NODE(brkStmt, BreakStatement);

	NEXT_TOKEN;
	Expect(TK_SEMICOLON);

	return (AstStatement)brkStmt;
}

/**
 *  continue-statement:
 *		continue ;
 */
static AstStatement ParseContinueStatement(void)
{
	AstContinueStatement contStmt;

	CREATE_AST_NODE(contStmt, ContinueStatement);

	NEXT_TOKEN;
	Expect(TK_SEMICOLON);

	return (AstStatement)contStmt;
}

/**
 *  return-statement:
 *		return [expression] ;
 */
static AstStatement ParseReturnStatement(void)
{
	AstReturnStatement retStmt;

	CREATE_AST_NODE(retStmt, ReturnStatement);

	NEXT_TOKEN;
	if (CurrentToken != TK_SEMICOLON)
	{
		retStmt->expr = ParseExpression();
	}
	Expect(TK_SEMICOLON);

	return (AstStatement)retStmt;
}

/**
 *  compound-statement:
 *		{ [declaration-list] [statement-list] }
 *  declaration-list:
 *		declaration
 *		declaration-list declaration
 *  statement-list:
 *		statement
 *		statement-list statement
 */
AstStatement ParseCompoundStatement(void)
{
	AstCompoundStatement compStmt;
	AstNode *tail;

	Level++;
	CREATE_AST_NODE(compStmt, CompoundStatement);

	NEXT_TOKEN;
	tail = &compStmt->decls;
	while (CurrentTokenIn(FIRST_Declaration))
	{
		// for example, 	"f(20,30);",  f is an id;  but f(20,30) is a statement, not declaration.
		if (CurrentToken == TK_ID && ! IsTypeName(CurrentToken))
			break;
		*tail = (AstNode)ParseDeclaration();
		tail = &(*tail)->next;
	}
	tail = &compStmt->stmts;
	while (CurrentToken != TK_RBRACE && CurrentToken != TK_END)
	{
		*tail = (AstNode)ParseStatement();
		tail = &(*tail)->next;
		if (CurrentToken == TK_RBRACE)
			break;
		SkipTo(FIRST_Statement, "the beginning of a statement");
	}
	Expect(TK_RBRACE);

	PostCheckTypedef();
	Level--;

	return (AstStatement)compStmt;
}

/**
 *  statement:
 *		expression-statement
 *		labeled-statement
 *		case-statement
 *		default-statement
 *		if-statement
 *		switch-statement
 *		while-statement
 *		do-statement
 *		for-statement
 *		goto-statement
 *		continue-statement
 *		break-statement
 *		return-statement
 *		compound-statement
 */
static AstStatement ParseStatement(void)
{
	switch (CurrentToken)
	{
	case TK_ID:
		/************************************
			In ParseLabelStatement(), ucl peek the next token
			to determine whether it is label-statement or expression-statement.
			(1)
				loopAgain :		---------->   Starting with ID
					...
					goto loopAgain

				or
			(2)
				f(20,30)		---------->	Starting with ID

				
		 ************************************/
		return ParseLabelStatement();

	case TK_CASE:
		return ParseCaseStatement();

	case TK_DEFAULT:
		return ParseDefaultStatement();

	case TK_IF:
		return ParseIfStatement();

	case TK_SWITCH:
		return ParseSwitchStatement();

	case TK_WHILE:
		return ParseWhileStatement();

	case TK_DO:
		return ParseDoStatement();

	case TK_FOR:
		return ParseForStatement();

	case TK_GOTO:
		return ParseGotoStatement();

	case TK_CONTINUE:
		return ParseContinueStatement();

	case TK_BREAK:
		return ParseBreakStatement();

	case TK_RETURN:
		return ParseReturnStatement();

	case TK_LBRACE:
		return ParseCompoundStatement();

	default:
		return ParseExpressionStatement();
	}
}
