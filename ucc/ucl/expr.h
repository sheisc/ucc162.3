#ifndef __EXPR_H_
#define __EXPR_H_
/***********************************************
 OPINFO(OP_LESS_EQ, 	  10,	"<=",	  Binary,		  JLE)
 OPINFO(OP_LSHIFT,		  11,	"<<",	  Binary,		  LSH)
 OPINFO(OP_RSHIFT,		  11,	">>",	  Binary,		  RSH)
 OPINFO(OP_ADD, 		  12,	"+",	  Binary,		  ADD)
 OPINFO(OP_SUB, 		  12,	"-",	  Binary,		  SUB)
 OPINFO(OP_MUL, 		  13,	"*",	  Binary,		  MUL)
 OPINFO(OP_DIV, 		  13,	"/",	  Binary,		  DIV)

 ***********************************************/
enum OP
{
#define OPINFO(op, prec, name, func, opcode) op,
#include "opinfo.h"
#undef OPINFO
};
/************************************
 Some tokens represent both unary and binary operator
e.g. 	*   +
 *************************************/
struct tokenOp
{
	// token used as binary operator
	int bop  : 16;
	// token used as unary operator
	int uop  : 16;
};
/******************************
	ty:	expression type
	op:	expression operator
	isarray:	array or not
	isfunc:	function or not
	lvalue:	lvalue or not
	bitfld:	bit-field or not
	inreg:	declared as register variable or not,  in register --> inreg
	kids:	expression operands
	val:		expression value
 ******************************/
struct astExpression
{
	AST_NODE_COMMON
	Type ty;
	int op : 16;
	int isarray : 1;
	int isfunc  : 1;
	int lvalue  : 1;
	int bitfld  : 1;
	int inreg   : 1;
	int unused  : 11;
	struct astExpression *kids[2];
	union value val;
};
// test whether it is a binary operator
#define IsBinaryOP(tok) (tok >= TK_OR && tok <= TK_MOD)
// token used as binary operator
#define	BINARY_OP       TokenOps[CurrentToken - TK_ASSIGN].bop
// token used as unary operator
#define UNARY_OP        TokenOps[CurrentToken - TK_ASSIGN].uop

int CanAssign(Type ty, AstExpression expr);
AstExpression Constant(struct coord coord, Type ty, union value val);
AstExpression Cast(Type ty, AstExpression expr);
AstExpression Adjust(AstExpression expr, int rvalue);
AstExpression DoIntegerPromotion(AstExpression expr);
AstExpression FoldConstant(AstExpression expr);
AstExpression FoldCast(Type ty, AstExpression expr);
AstExpression Not(AstExpression expr);
AstExpression CheckExpression(AstExpression expr);
AstExpression CheckConstantExpression(AstExpression expr);

void TranslateBranch(AstExpression expr, BBlock trueBB, BBlock falseBB);
Symbol TranslateExpression(AstExpression expr);

extern char* OPNames[];

#endif




