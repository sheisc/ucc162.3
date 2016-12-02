#include "ucl.h"
#include "ast.h"
#include "expr.h"


// binary operator
#define EXECUTE_BOP(op)                                                                      \
    if (tcode == I4) val.i[0] = expr1->val.i[0] op expr2->val.i[0];                          \
    else if (tcode == U4) val.i[0] = (unsigned)expr1->val.i[0] op (unsigned)expr2->val.i[0]; \
    else if (tcode == F4) val.f = expr1->val.f op expr2->val.f;                              \
    else if (tcode == F8) val.d = expr1->val.d op expr2->val.d;
// relational operator
#define EXECUTE_ROP(op)                                                                      \
    if (tcode == I4) val.i[0] = expr1->val.i[0] op expr2->val.i[0];                          \
    else if (tcode == U4) val.i[0] = (unsigned)expr1->val.i[0] op (unsigned)expr2->val.i[0]; \
    else if (tcode == F4) val.i[0] = expr1->val.f op expr2->val.f;                           \
    else if (tcode == F8) val.i[0] = expr1->val.d op expr2->val.d;

/**
 * Cast constant expression. e.g. (float)3
 */
// (float)3	----------->  3.0f		we do the calculation at compile time.
//@ty	T(FLOAT)
//@expr	3
AstExpression FoldCast(Type ty, AstExpression expr)
{
	// source type-code
	int scode = TypeCode(expr->ty);
	// destination type-code
	int dcode = TypeCode(ty);
	
	switch (scode)
	{
	case I1: case U1: case I2: case U2:
		/*******************************
			 int main(){
				 float a = (float) ((char)'x');
				 return 0;
			 }

			 	fold.c 32:  scode = 4  dcode = 4
				fold.c 32:  scode = 4  dcode = 0
				fold.c 32:  scode = 0  dcode = 4
				fold.c 32:  scode = 4  dcode = 6
		Here, doing the following is useless:
			expr->val.i[0] = expr->val.i[0];					
		So nothing is done here.

		The actual extending work will be done
		in Cast() and EmitCast() X86_EXTI1, ....
		 *******************************/		
		assert(dcode == I4);
		break;

	case I4:
		if (dcode == F4)
		{
			expr->val.f = (float)expr->val.i[0];
		}
		else if (dcode == F8)
		{
			expr->val.d = (double)expr->val.i[0];
		}
		else if (dcode == I1)
		{
			expr->val.i[0] = (char)expr->val.i[0];
		}
		else if (dcode == U1)
		{
			expr->val.i[0] = (unsigned char)expr->val.i[0];
		}
		else if (dcode == I2)
		{
			expr->val.i[0] = (short)expr->val.i[0];
		}
		else if (dcode == U2)
		{
			expr->val.i[0] = (unsigned short)expr->val.i[0];
		}
		break;

	case U4:
		if (dcode == F4)
		{
			expr->val.f = (float)(unsigned)expr->val.i[0];
		}
		else if (dcode == F8)
		{
			expr->val.d = (double)(unsigned)expr->val.i[0];
		}
		break;

	case F4:
		if (dcode == I4)
		{
			expr->val.i[0] = (int)expr->val.f;
		}
		else if (dcode == U4)
		{
			expr->val.i[0] = (unsigned)expr->val.f;
		}
		else
		{
			expr->val.d = (double)expr->val.f;
		}
		break;

	case F8:
		if (dcode == I4)
		{
			expr->val.i[0] = (int)expr->val.d;
		}
		else if (dcode == U4)
		{
			expr->val.i[0] = (unsigned)expr->val.d;
		}
		else
		{
			expr->val.f = (float)expr->val.d;
		}
		break;

	default:
		assert(0);
	}
	//PRINT_DEBUG_INFO(("%s",TypeToString(ty)));
	expr->ty = ty;
	return expr;
}
/********************************************************
	Construct a new value to represent a const value.
 ********************************************************/
AstExpression Constant(struct coord coord, Type ty, union value val)
{
	AstExpression expr;

	CREATE_AST_NODE(expr, Expression);
	expr->coord = coord;
	expr->ty = ty;
	expr->op = OP_CONST;
	expr->val = val;

	return expr;
}

/**
 * Constant folding. e.g. 3 + 4
 */
AstExpression FoldConstant(AstExpression expr)
{
	// type code, see TypeCode().
	int tcode;
	union value val;
	AstExpression expr1, expr2;
	/************************************************************
		For binary operator, if one operand is not const,
			it can't be folded.
	 ************************************************************/
	if (expr->op >= OP_OR && expr->op <= OP_MOD &&
	    ! (expr->kids[0]->op == OP_CONST && expr->kids[1]->op == OP_CONST))
		return expr;
	/*********************************************************
		for unary operator, if its soly operand is not const,
			it cant' folded.			
	 *********************************************************/
	if (expr->op >= OP_POS && expr->op <= OP_NOT && expr->kids[0]->op != OP_CONST)
		return expr;
	//  300 ?  a : b   ----->    a 
	if (expr->op == OP_QUESTION)
	{
		if (expr->kids[0]->op == OP_CONST && IsIntegType(expr->kids[0]->ty))
		{
			return expr->kids[0]->val.i[0] ? expr->kids[1]->kids[0] :expr->kids[1]->kids[1];
		}
		return expr;
	}
	/*********************************************
		Do the constant calculation at compile time.
	 *********************************************/
	val.i[1] = val.i[0] = 0;
	tcode = TypeCode(expr->kids[0]->ty);
	expr1 = expr->kids[0];
	expr2 = expr->kids[1];
	switch (expr->op)
	{
	case OP_OR:		// "||"
		EXECUTE_ROP(||);
		break;

	case OP_AND:	// "&&"
		EXECUTE_ROP(&&);
		break;

	case OP_BITOR:
		val.i[0] = expr1->val.i[0] | expr2->val.i[0];
		break;

	case OP_BITXOR:
		val.i[0] = expr1->val.i[0] ^ expr2->val.i[0];
		break;

	case OP_BITAND:
		val.i[0] = expr1->val.i[0] & expr2->val.i[0];
		break;

	case OP_EQUAL:
		EXECUTE_ROP(==);
		break;

	case OP_UNEQUAL:
		EXECUTE_ROP(!=);
		break;

	case OP_GREAT:
		EXECUTE_ROP(>);
		break;

	case OP_LESS:
		EXECUTE_ROP(<);
		break;

	case OP_GREAT_EQ:
		EXECUTE_ROP(>=);
		break;

	case OP_LESS_EQ:
		EXECUTE_ROP(<=);
		break;

	case OP_LSHIFT:
		/*********************************************
			see examples/foldconst/diff.c.
			Some warnings here may be welcome.
		 *********************************************/
		val.i[0] = expr1->val.i[0] << expr2->val.i[0];
		break;

	case OP_RSHIFT:
		if (tcode == U4)
			val.i[0] = (unsigned)expr1->val.i[0] >> expr2->val.i[0];
		else
			val.i[0] = expr1->val.i[0] >> expr2->val.i[0];
		break;

	case OP_ADD:
		EXECUTE_BOP(+);
		break;
		
	case OP_SUB:
		EXECUTE_BOP(-);
		break;

	case OP_MUL:
		EXECUTE_BOP(*);
		break;

	case OP_DIV:
		EXECUTE_BOP(/);
		break;

	case OP_MOD:	// %
		if (tcode == U4)
			val.i[0] = (unsigned)expr1->val.i[0] % expr2->val.i[0];
		else
			val.i[0] = expr1->val.i[0] % expr2->val.i[0];
		break;

	case OP_NEG:	// -12, -12.3
		if (tcode == I4 || tcode == U4){
			val.i[0] = -expr1->val.i[0];
		}else if (tcode == F4){
			val.f = -expr1->val.f;	
		}
		else if (tcode == F8){
			val.d = -expr1->val.d;	
		}
		break;

	case OP_COMP:	// ~0xABCD
		val.i[0] = ~expr1->val.i[0];
		break;

	case OP_NOT:	// !
		if (tcode == I4 || tcode == U4)
			val.i[0] = ! expr1->val.i[0];
		else if (tcode == F4)
			val.i[0] = ! expr1->val.f;
		else if (tcode == F8)
			val.i[0] = ! expr1->val.d;
		break;

	default:
		assert(0);
		return expr;
	}
	/****************************************
		When we are here, 
			the value of const-expression have been calculated at compile time.
		We construct a new const value to represent the result of the original const-expression.
	 ****************************************/
	return Constant(expr->coord, expr->ty, val);
}

