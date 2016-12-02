#include "ucl.h"
#include "ast.h"
#include "expr.h"

/**
 * This module parses the C expression.
 */

/**
 * The token's corresponding unary or binary opearator.
 * Some tokens represent both unary and binary operator,
 * e.g. * or +.
 */
 /*********************************************
 TOKENOP(TK_LSHIFT,        OP_LSHIFT,        OP_NONE)
TOKENOP(TK_RSHIFT,        OP_RSHIFT,        OP_NONE)
TOKENOP(TK_ADD,           OP_ADD,           OP_POS)
TOKENOP(TK_SUB,           OP_SUB,           OP_NEG)
TOKENOP(TK_MUL,           OP_MUL,           OP_DEREF)
  *********************************************/
static struct tokenOp TokenOps[] = 
{
#define TOKENOP(tok, bop, uop) {bop, uop},
#include "tokenop.h"
#undef  TOKENOP
};

// operators' precedence
static int Prec[] =
{
#define OPINFO(op, prec, name, func, opcode) prec,
#include "opinfo.h"
	0
#undef OPINFO
};

// expression for integer constant 0
AstExpression Constant0;

/********************************************************
 OPINFO(OP_COMMA,		  1,	",",	  Comma,		  NOP)
 OPINFO(OP_ASSIGN,		  2,	"=",	  Assignment,	  NOP)
 OPINFO(OP_BITOR_ASSIGN,  2,	"|=",	  Assignment,	  NOP)
 ********************************************************/
// operator names, mainly used for error reporting
char *OPNames[] = 
{
#define OPINFO(op, prec, name, func, opcode) name,
#include "opinfo.h"
	NULL
#undef OPINFO
};

/**
 *  primary-expression:
 *		ID
 *		constant
 *		string-literal
 *		( expression )
 */
static AstExpression ParsePrimaryExpression(void)
{
	AstExpression expr;


	switch (CurrentToken)
	{
	case TK_ID:

		CREATE_AST_NODE(expr, Expression);

		expr->op = OP_ID;
		expr->val = TokenValue;
		NEXT_TOKEN;

		return expr;

	/// Notice: Only when parsing constant and string literal,
	/// ty member in astExpression is used since from OP_CONST
	/// and OP_STR alone the expression's type can't be determined
	// 20, 20U,20LU, 3.0f, 5.0, ....
	case TK_INTCONST:		// 12345678
	case TK_UINTCONST:		// 12345678U
	case TK_LONGCONST:		// 12345678L
	case TK_ULONGCONST:		// 12345678UL
	case TK_LLONGCONST:		// 12345678LL
	case TK_ULLONGCONST:	// 12345678ULL
	case TK_FLOATCONST:		// 123.456f  123.456F
	case TK_DOUBLECONST:	// 123.456
	case TK_LDOUBLECONST:	// 123.456L	123.456l

		CREATE_AST_NODE(expr, Expression);
		/***************************************************
			see type.h  , enum{...,ULONGLONG,ENUM,FLOAT,}
		 ***************************************************/
		if (CurrentToken >= TK_FLOATCONST)
			CurrentToken++;

		/// nasty, requires that both from TK_INTCONST to TK_LDOUBLECONST
		/// and from INT to LDOUBLE are consecutive
		expr->ty = T(INT + CurrentToken - TK_INTCONST);
		expr->op = OP_CONST;
		expr->val = TokenValue;
		NEXT_TOKEN;

		return expr;

	case TK_STRING:			// "ABC"
	case TK_WIDESTRING:		// L"ABC"

		CREATE_AST_NODE(expr, Expression);

		expr->ty = ArrayOf(((String)TokenValue.p)->len + 1, CurrentToken == TK_STRING ? T(CHAR) : WCharType);
		expr->op = OP_STR;
		expr->val = TokenValue;
		NEXT_TOKEN;

		return expr;

	case TK_LPAREN:		// (expr)

		NEXT_TOKEN;
		expr = ParseExpression();
		Expect(TK_RPAREN);

		return expr;

	default:
		Error(&TokenCoord, "Expect identifier, string, constant or (");
		return Constant0;
	}
}

/**
 *  postfix-expression:
 *		primary-expression
 *		postfix-expression [ expression ]
 *		postfix-expression ( [argument-expression-list] )
 *		postfix-expression . identifier
 *		postfix-expression -> identifier
 *		postfix-expression ++
 *		postfix-expression --
 */
static AstExpression ParsePostfixExpression(void)
{
	AstExpression expr, p;

	expr = ParsePrimaryExpression();

	while (1)
	{
		switch (CurrentToken)
		{
		case TK_LBRACKET:	// postfix-expression [ expression ]

			CREATE_AST_NODE(p, Expression);

			p->op = OP_INDEX;
			p->kids[0] = expr;
			NEXT_TOKEN;
			p->kids[1] = ParseExpression();
			Expect(TK_RBRACKET);

			expr = p;
			break;

		case TK_LPAREN:		// postfix-expression ( [argument-expression-list] )

			CREATE_AST_NODE(p, Expression);

			p->op = OP_CALL;
			p->kids[0] = expr;
			NEXT_TOKEN;
			if (CurrentToken != TK_RPAREN)
			{
				AstNode *tail;

				/// function call expression's second kid is actually
				/// a list of expression instead of a single expression
				p->kids[1] = ParseAssignmentExpression();
				tail = &p->kids[1]->next;
				while (CurrentToken == TK_COMMA)
				{
					NEXT_TOKEN;
					*tail = (AstNode)ParseAssignmentExpression();
					tail = &(*tail)->next;
				}
			}
			Expect(TK_RPAREN);

			expr = p;
			break;

		case TK_DOT:		// postfix-expression . identifier
		case TK_POINTER:	// postfix-expression -> identifier

			CREATE_AST_NODE(p, Expression);

			p->op = (CurrentToken == TK_DOT ? OP_MEMBER : OP_PTR_MEMBER);
			p->kids[0] = expr;
			NEXT_TOKEN;
			if (CurrentToken != TK_ID)
			{
				Error(&p->coord, "Expect identifier as struct or union member");
			}
			else
			{
				p->val = TokenValue;
				NEXT_TOKEN;
			}

			expr = p;
			break;

		case TK_INC:	// postfix-expression ++
		case TK_DEC:	// postfix-expression --

			CREATE_AST_NODE(p, Expression);

			p->op = (CurrentToken == TK_INC) ? OP_POSTINC : OP_POSTDEC;
			p->kids[0] = expr;
			NEXT_TOKEN;

			expr = p;
			break;

		default:

			return expr;
		}
	}
}

/**
 *  unary-expression:
 *		postfix-expression
 *		unary-operator unary-expression
 *		( type-name ) unary-expression
 *		sizeof unary-expression
 *		sizeof ( type-name )
 *
 *  unary-operator:
 *		++ -- & * + - ! ~
 */
 // The grammar of unary-expression in UCC is a little different from C89.
 // There is no cast-expression in UCC.
static AstExpression ParseUnaryExpression()
{
	AstExpression expr;
	int t;

	switch (CurrentToken)
	{
	case TK_INC:
	case TK_DEC:
	case TK_BITAND:
	case TK_MUL:
	case TK_ADD:
	case TK_SUB:
	case TK_NOT:
	case TK_COMP:

		CREATE_AST_NODE(expr, Expression);

		expr->op = UNARY_OP;
		NEXT_TOKEN;
		expr->kids[0] = ParseUnaryExpression();

		return expr;

	case TK_LPAREN:

		/// When current token is (, it may be a type cast expression
		/// or a primary expression, we need to look ahead one token,
		/// if next token is type name, the expression is treated as
		/// a type cast expression; otherwise a primary expresion
		BeginPeekToken();
		t = GetNextToken();
		if (IsTypeName(t))
		{
			EndPeekToken();

			CREATE_AST_NODE(expr, Expression);

			expr->op = OP_CAST;
			NEXT_TOKEN;
			expr->kids[0] =  (AstExpression)ParseTypeName();
			Expect(TK_RPAREN);
			expr->kids[1] = ParseUnaryExpression();

			return expr;
		}
		else
		{
			EndPeekToken();
			return ParsePostfixExpression();
		}
		break;

	case TK_SIZEOF:

		/// this case hase the same issue with TK_LPAREN case
		CREATE_AST_NODE(expr, Expression);

		expr->op = OP_SIZEOF;
		NEXT_TOKEN;
		if (CurrentToken == TK_LPAREN)
		{			
			BeginPeekToken();
			t = GetNextToken();
			// PRINT_DEBUG_INFO(("case TK_SIZEOF:"));
			if (IsTypeName(t))
			{
				//  sizeof ( type-name )  -------------->  () is required here.
				// sizeof(a)		sizeof  a
				EndPeekToken();
				
				NEXT_TOKEN;
				/// In this case, the first kid is not an expression,
				/// but thanks to both type name and expression have a 
				/// kind member to discriminate them.
				expr->kids[0] = (AstExpression)ParseTypeName();
				Expect(TK_RPAREN);
			}
			else
			{
				// sizeof unary-expression
				EndPeekToken();
				expr->kids[0] = ParseUnaryExpression();
			}
		}
		else
		{
			// sizeof unary-expression
			expr->kids[0] = ParseUnaryExpression();
		}

		return expr;

	default:
		return ParsePostfixExpression();
	}
}

/**
 * Parse a binary expression, from logical-OR-expresssion to multiplicative-expression
 */
/*****************************************************************
	@prec
		The precedence of Outer-Most operator of an binary expression

	This function will parse the sub-expression with outer-most operator of @ prec precedence.
		
		L4:		------>  ParseBinaryExpression(Prec[OP_OR]);
			(The precedence of Outer-Most operator of L4  is Prec[OP_OR])
			L5			||	L5	||	L5	||	...	|| L5
		L5:		------>  ParseBinaryExpression(Prec[OP_AND]);	
			L6			&&	L6	&&	...			&& L6
		L6:
			L7			|	L7	|	...			|	L7
		...........
		L13:	------> ParseBinaryExpression(Prec[OP_MUL]);
			unary-expr			*	unary-expr	*	...		* unary-expr	
		
 *****************************************************************/
static AstExpression ParseBinaryExpression(int prec)
{
	AstExpression binExpr;
	AstExpression expr;
	int newPrec;
	/************************************************
	Suppose @prec is precedence of '+'.
		(1) 		...	+ 
						b*c*d+d
		(2)		...	+ 
						b  ||c	
		(3) 		...	+ 
						b	+ c
	No mater what the @prec is , one thing is sure:
		the sub-expression we are parsing begins with unary-expression.		
	 ************************************************/
	expr = ParseUnaryExpression();
	/************************************************
	Right now,
		(1) 		a	+b*c*d+d
				a	+	b *		-------------------------> we are here
							c*d+d
		(2)		a	+	b  ||		--------------------------> we are here
							c
		(3) 		a	+b	+ c
				a	+	b  +	-------------------------> we are here
							 c					
			Suppose  @prec  is '+',	
				we are parsing sub-expression (call it SubE for later discussion)
				with '+' as outer-most operator.
			when the following operator is '*' or '+',
				the following string is also part of SubE.
			when the following is '||', which is lower than '+',
				it is not part of SubE.
	 ************************************************/	
	/// while the following binary operater's precedence is higher than current
	/// binary operator's precedence, parses a higer precedence expression
	while (IsBinaryOP(CurrentToken) && (newPrec = Prec[BINARY_OP]) >= prec)
	{
		CREATE_AST_NODE(binExpr, Expression);
		/************************************************
			(1) 	a+b*c*d+d
					a	+	-------------------------> we are here
							b*c*d+d
			(2)
			
					a	+	--------------------------> we are here
							b+c
		 ************************************************/

		binExpr->op = BINARY_OP;
		binExpr->kids[0] = expr;
		NEXT_TOKEN;

		binExpr->kids[1] = ParseBinaryExpression(newPrec + 1);
		/**********************************************
			Try to loop again to find whether binExpr could be 
			the left operand of the Current Operator when we are here.
		 **********************************************/
		expr = binExpr;
	}

	return expr;

}

/**
 *  conditional-expression:
 *      logical-OR-expression
 *      logical-OR-expression ? expression : conditional-expression
 */
static AstExpression ParseConditionalExpression(void)
{
	AstExpression expr;

	expr = ParseBinaryExpression(Prec[OP_OR]);
	if (CurrentToken == TK_QUESTION)
	{
		AstExpression condExpr;

		CREATE_AST_NODE(condExpr, Expression);

		condExpr->op = OP_QUESTION;
		condExpr->kids[0] = expr;
		NEXT_TOKEN;

		CREATE_AST_NODE(condExpr->kids[1], Expression);

		condExpr->kids[1]->op = OP_COLON;
		condExpr->kids[1]->kids[0] = ParseExpression();
		Expect(TK_COLON);
		condExpr->kids[1]->kids[1] = ParseConditionalExpression();

		return condExpr;
	}

	return expr;
}

/**
 *  assignment-expression:
 *      conditional-expression
 *      unary-expression assignment-operator assignment-expression
 *  assignment-operator:
 *      = *= /= %= += -= <<= >>= &= ^= |=
 *  There is a little twist here: the parser always treats the first nonterminal
 *  as a conditional expression.
 */
AstExpression ParseAssignmentExpression(void)
{
	AstExpression expr;
	/***********************************************
		It is not accurate here.
		We will check it during semantics .
	 ***********************************************/
	expr = ParseConditionalExpression();
	/***************************************************************
	see token.h
			TOKEN(TK_ASSIGN,        "=")
			TOKEN(TK_BITOR_ASSIGN,  "|=")
			TOKEN(TK_BITXOR_ASSIGN, "^=")
			TOKEN(TK_BITAND_ASSIGN, "&=")
			TOKEN(TK_LSHIFT_ASSIGN, "<<=")
			TOKEN(TK_RSHIFT_ASSIGN, ">>=")
			TOKEN(TK_ADD_ASSIGN,    "+=")
			TOKEN(TK_SUB_ASSIGN,    "-=")
			TOKEN(TK_MUL_ASSIGN,    "*=")
			TOKEN(TK_DIV_ASSIGN,    "/=")
			TOKEN(TK_MOD_ASSIGN,    "%=")	
	 ***************************************************************/
	if (CurrentToken >= TK_ASSIGN && CurrentToken <= TK_MOD_ASSIGN)
	{
		AstExpression asgnExpr;

		CREATE_AST_NODE(asgnExpr, Expression);

		asgnExpr->op = BINARY_OP;
		asgnExpr->kids[0] = expr;
		NEXT_TOKEN;
		asgnExpr->kids[1] = ParseAssignmentExpression();

		return asgnExpr;
	}

	return expr;
}

/**
 *  expression:
 *      assignment-expression
 *      expression , assignment-expression
 */
AstExpression ParseExpression(void)
{
	AstExpression expr, comaExpr;

	expr = ParseAssignmentExpression();
	while(CurrentToken == TK_COMMA)
	{
		CREATE_AST_NODE(comaExpr, Expression);

		comaExpr->op = OP_COMMA;
		comaExpr->kids[0] = expr;
		NEXT_TOKEN;
		comaExpr->kids[1] = ParseAssignmentExpression();

		expr = comaExpr;
	}

	return expr;
}

/**
 * Parse constant expression which is actually a conditional expression
 */ 
//	constant-expression:	conditional-expression 
AstExpression ParseConstantExpression(void)
{
	return ParseConditionalExpression();
}


