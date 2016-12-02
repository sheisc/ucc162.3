#include "ucl.h"
#include "ast.h"
#include "decl.h"
#include "expr.h"
#include "stmt.h"

#define PushStatement(v, stmt)   INSERT_ITEM(v, stmt)
#define PopStatement(v)          (v->data[--v->len])
#define TopStatement(v)          TOP_ITEM(v)

static AstStatement CheckStatement(AstStatement stmt);
// If not existing, add the label.
static Label TryAddLabel(char *id)
{
	Label p = CURRENTF->labels;

	while (p)
	{
		if (p->id == id)
			return p;
		p = p->next;
	}

	CALLOC(p);

	p->id = id;
	p->next = CURRENTF->labels;
	CURRENTF->labels = p;

	return p;
}

static void AddCase(AstSwitchStatement swtchStmt, AstCaseStatement caseStmt)
{
	AstCaseStatement p = swtchStmt->cases;
	AstCaseStatement *pprev = &swtchStmt->cases;
	int diff;
	/*******************************************************
		sort the caseStatements ?
	 *******************************************************/
	while (p)
	{
		diff = caseStmt->expr->val.i[0] - p->expr->val.i[0];
		if (diff < 0)
			break;

		if (diff > 0)
		{
			pprev = &p->nextCase;
			p = p->nextCase;
		}
		else
		{
			Error(&caseStmt->coord, "Repeated constant in a switch statement");
			return;
		}
	}

	*pprev = caseStmt;
	caseStmt->nextCase = p;
}
/***************************************************************
 Syntax
 
		   expression-statement:
				   expression<opt> ;
 
 Semantics
 
	The expression in an expression statement is evaluated as a void
 expression for its side effects./66/
 
	A null statement (consisting of just a semicolon) performs no
 operations.

 ***************************************************************/
static AstStatement CheckExpressionStatement(AstStatement stmt)
{
	AstExpressionStatement exprStmt = AsExpr(stmt);

	if (exprStmt->expr != NULL)
	{
		exprStmt->expr = CheckExpression(exprStmt->expr);
	}

	return stmt;
}
/**************************************************************************
 Syntax 
		   labeled-statement:
				   identifier :  statement
				   case  constant-expression :	statement
				   default :  statement
 ***************************************************************************/
 //  only check   "id: statement"  in this function.   case-label is checked later.
static AstStatement CheckLabelStatement(AstStatement stmt)
{
	AstLabelStatement labelStmt = AsLabel(stmt);

	labelStmt->label = TryAddLabel(labelStmt->id);
	if (labelStmt->label->defined)
	{
		/*****************************************************
		int main(){
		Again:
		Again:
			return 0;
		}
		 *****************************************************/
		Error(&stmt->coord, "Label name should be unique within function.");
	}
	labelStmt->label->defined = 1;
	labelStmt->label->coord = stmt->coord;
	labelStmt->stmt = CheckStatement(labelStmt->stmt);

	return stmt;
}
/*****************************************************************
case-statement:
				case  constant-expression :	statement
 Constraints 
	A case or default label shall appear only in a switch statement.
 Further constraints on such labels are discussed under the switch
 statement.

 Even the following code is legal:
 	int a = 5;
	switch(a){
		printf("123.\n");	---------- these are dead-code.
		break;
	case 3:
		printf("3.\n");
		printf("this printf is not part of case-statment from syntactic view.\n");
	case 2:
		printf("2.\n");
	}
 *****************************************************************/
static AstStatement CheckCaseStatement(AstStatement stmt)
{
	AstCaseStatement caseStmt = AsCase(stmt);
	AstSwitchStatement swtchStmt;
	// We have pushed the current switch statement in CheckSwitchStatement(...)
	swtchStmt = (AstSwitchStatement)TopStatement(CURRENTF->swtches);
	if (swtchStmt == NULL)
	{
		Error(&stmt->coord, "A case label shall appear in a switch statement.");
		return stmt;
	}

	caseStmt->expr = CheckConstantExpression(caseStmt->expr);
	if (caseStmt->expr == NULL)
	{
		Error(&stmt->coord, "The case value must be integer constant.");
		return stmt;
	}

	caseStmt->stmt = CheckStatement(caseStmt->stmt);
	caseStmt->expr = FoldCast(swtchStmt->expr->ty, caseStmt->expr);
	AddCase(swtchStmt, caseStmt);

	return stmt;
}
/******************************************************************
	default-statement
			default :  statement
 ******************************************************************/
static AstStatement CheckDefaultStatement(AstStatement stmt)
{
	AstDefaultStatement defStmt = AsDef(stmt);
	AstSwitchStatement swtchStmt;

	swtchStmt = (AstSwitchStatement)TopStatement(CURRENTF->swtches);
	if (swtchStmt == NULL)
	{
		Error(&stmt->coord, "A default label shall appear in a switch statement.");
		return stmt;
	}
	if (swtchStmt->defStmt != NULL)
	{
		Error(&stmt->coord, "There shall be only one default label in a switch statement.");
		return stmt;
	}

	defStmt->stmt = CheckStatement(defStmt->stmt);
	swtchStmt->defStmt = defStmt;

	return stmt;
}
/*****************************************************************
	if-statement
		     if (  expression )  statement
                  if (  expression )  statement else  statement
 *****************************************************************/
static AstStatement CheckIfStatement(AstStatement stmt)
{
	AstIfStatement ifStmt = AsIf(stmt);

	ifStmt->expr = Adjust(CheckExpression(ifStmt->expr), 1);
	// The controlling expression of an if statement shall have scalar type. 
	if (! IsScalarType(ifStmt->expr->ty))
	{
		Error(&stmt->coord, "The expression in if statement shall be scalar type.");
	}

	ifStmt->thenStmt = CheckStatement(ifStmt->thenStmt);
	if (ifStmt->elseStmt)
	{
		ifStmt->elseStmt = CheckStatement(ifStmt->elseStmt);
	}

	return stmt;
}
/**************************************************************************
switch-statement
	 switch (  expression )  statement

A deadly-bug in UCC:
	int main(){
		int a = 3;
		switch(a){
		case 2:	
		case 3:		
			break;
		}
		return 0;
	}
 **************************************************************************/
static AstStatement CheckSwitchStatement(AstStatement stmt)
{
	AstSwitchStatement swtchStmt = AsSwitch(stmt);
	// switch statement is breakable
	PushStatement(CURRENTF->swtches,   stmt);
	PushStatement(CURRENTF->breakable, stmt);

	swtchStmt->expr = Adjust(CheckExpression(swtchStmt->expr), 1);

	if (! IsIntegType(swtchStmt->expr->ty))
	{
		Error(&stmt->coord, "The expression in a switch statement shall be integer type.");
		swtchStmt->expr->ty = T(INT);
	}
	if (swtchStmt->expr->ty->categ < INT)
	{                               
		swtchStmt->expr = Cast(T(INT), swtchStmt->expr);
	}
	swtchStmt->stmt = CheckStatement(swtchStmt->stmt);

	PopStatement(CURRENTF->swtches);
	PopStatement(CURRENTF->breakable);

	return stmt;
}
/**************************************************************
 iteration-statement:
		 while (  expression )	statement
		 do  statement while (	expression ) ;
 **************************************************************/
static AstStatement CheckLoopStatement(AstStatement stmt)
{
	AstLoopStatement loopStmt = AsLoop(stmt);

	PushStatement(CURRENTF->loops,    stmt);
	PushStatement(CURRENTF->breakable, stmt);
	// Adjust expr's type to Pointer(...)  when its type is FUNCTION/ARRAY
	loopStmt->expr = Adjust(CheckExpression(loopStmt->expr), 1);
	if (! IsScalarType(loopStmt->expr->ty))
	{
		Error(&stmt->coord, "The expression in do or while statement shall be scalar type.");
	}
	loopStmt->stmt = CheckStatement(loopStmt->stmt);
	
	PopStatement(CURRENTF->loops);
	PopStatement(CURRENTF->breakable);

	return stmt;
}
/************************************************************
for-statement:
 	for (	expression-1 ;	expression-2 ;	expression-3 )	statement

 ************************************************************/
static AstStatement CheckForStatement(AstStatement stmt)
{
	AstForStatement forStmt = AsFor(stmt);

	PushStatement(CURRENTF->loops,     stmt);
	PushStatement(CURRENTF->breakable, stmt);
	// check expression-1
	if (forStmt->initExpr)
	{
		forStmt->initExpr = CheckExpression(forStmt->initExpr);
	}
	// check expression-2
	if (forStmt->expr)
	{
		/********************************************************
			The reason we Adjust(expression-2) here is that 
				if it is a FUNCTION/ARRAY, it will be converted to POINTER(FUNCTION).
				So the adjusted expression can be used for judging.

				Pointer(...) is also considered a Scalar-type.
		 ********************************************************/
		forStmt->expr = Adjust(CheckExpression(forStmt->expr), 1);
		if (! IsScalarType(forStmt->expr->ty))
		{
			Error(&stmt->coord, "The second expression in for statement shall be scalar type.");
		}
	}
	// check expression-3
	if (forStmt->incrExpr)
	{
		forStmt->incrExpr = CheckExpression(forStmt->incrExpr);
	}
	forStmt->stmt = CheckStatement(forStmt->stmt);

	PopStatement(CURRENTF->loops);
	PopStatement(CURRENTF->breakable);

	return stmt;
}
/**************************************************************
goto-statement:
	goto  identifier ;
 **************************************************************/
static AstStatement CheckGotoStatement(AstStatement stmt)
{
	AstGotoStatement gotoStmt = AsGoto(stmt);

	if (gotoStmt->id != NULL)
	{
		gotoStmt->label = TryAddLabel(gotoStmt->id);
		gotoStmt->label->coord = gotoStmt->coord;
		gotoStmt->label->ref++;
	}

	return stmt;
}
// break;
static AstStatement CheckBreakStatement(AstStatement stmt)
{
	AstBreakStatement brkStmt = AsBreak(stmt);

	brkStmt->target = TopStatement(CURRENTF->breakable);
	if (brkStmt->target == NULL)
	{
		Error(&stmt->coord, "The break shall appear in a switch or loop");
	}

	return stmt;
}
// continue;
static AstStatement CheckContinueStatement(AstStatement stmt)
{
	AstContinueStatement contStmt = AsCont(stmt);

	contStmt->target = (AstLoopStatement)TopStatement(CURRENTF->loops);
	if (contStmt->target == NULL)
	{
		Error(&stmt->coord, "The continue shall appear in a loop.");
	}

	return stmt;
}
/***************************************************************
	return-statement
			 return  expression<opt> ;
 ***************************************************************/
static AstStatement CheckReturnStatement(AstStatement stmt)
{
	AstReturnStatement retStmt = AsRet(stmt);
	Type rty = FSYM->ty->bty;
	/**********************************************************
		If there is one return-statement,
			the current function is considered to return correctly.
		For example:
		int main(){
			int a = 0;
			if(a){
				printf("f.\n");
				return 0;
			}
		}		
	 **********************************************************/
	CURRENTF->hasReturn = 1;
	if (retStmt->expr)
	{
		retStmt->expr = Adjust(CheckExpression(retStmt->expr), 1);

		if (rty->categ == VOID)
		{
			Error(&stmt->coord, "Void function should not return value");
			return stmt;
		}

		if (! CanAssign(rty, retStmt->expr))
		{
			Error(&stmt->coord, "Incompatible return value");
			return stmt;
		}

		retStmt->expr = Cast(rty, retStmt->expr);

		return stmt;
	}

	if (rty->categ != VOID)
	{
		Warning(&stmt->coord, "The function should return a value.");
	}
	return stmt;
}
/********************************************************************
(1)
	In fact , it is compound-statment, we do EnterScope()/ExitScope() here.
	But it is compound-statment defined in the function, that is ,local.
(2)  From syntactic view, the compound-statment in the function-definion and 
	local compound-statement are the same.
(3)  From semantic point, we are checking the compound-statement of function-definition
	we have something more to do after we EnterScope().
	see void CheckFunction(AstFunction func)	for detail.
	

 ********************************************************************/
static AstStatement CheckLocalCompound(AstStatement stmt)
{
	AstStatement s;

	EnterScope();

	s = CheckCompoundStatement(stmt);

	ExitScope();

	return stmt;
}

static AstStatement (* StmtCheckers[])(AstStatement) = 
{
	CheckExpressionStatement,	// the index is (NK_ExpressionStatement - NK_ExpressionStatement), 0
	CheckLabelStatement,			// the index is ( NK_LabelStatement - NK_ExpressionStatement), 1
	CheckCaseStatement,			// 2
	CheckDefaultStatement,		// 3
	CheckIfStatement,			// 4
	CheckSwitchStatement,		// 5
	CheckLoopStatement,
	CheckLoopStatement,
	CheckForStatement,
	CheckGotoStatement,
	CheckBreakStatement,
	CheckContinueStatement,
	CheckReturnStatement,
	CheckLocalCompound
};

static AstStatement CheckStatement(AstStatement stmt)
{
	return (* StmtCheckers[stmt->kind - NK_ExpressionStatement])(stmt);
}
/*************************************************************
 3.6.2 Compound statement, or block
 
 Syntax 
		   compound-statement:
				   {  declaration-list<opt> statement-list<opt> } 
		   declaration-list:
				   declaration
				   declaration-list declaration 
		   statement-list:
				   statement
				   statement-list statement
 
 Semantics
 
	A compound statement (also called a block )allows a set of
 statements to be grouped into one syntactic unit, which may have its
 own set of declarations and initializations (as discussed in
 $3.1.2.4).  The initializers of objects that have automatic storage
 duration are evaluated and the values are stored in the objects in the
 order their declarators appear in the translation unit.

 **************************************************************/
AstStatement CheckCompoundStatement(AstStatement stmt)
{
	AstCompoundStatement compStmt = AsComp(stmt);
	AstNode p;

	compStmt->ilocals = CreateVector(1);
	p = compStmt->decls;
	while (p)
	{
		CheckLocalDeclaration((AstDeclaration)p, compStmt->ilocals);
		p = p->next;
	}
	p = compStmt->stmts;
	while (p)
	{
		CheckStatement((AstStatement)p);
		p = p->next;
	}

	return stmt;
}
