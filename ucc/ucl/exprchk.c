#include "ucl.h"
#include "ast.h"
#include "expr.h"
#include "decl.h"

#define SWAP_KIDS(expr)              \
{                                    \
    AstExpression t = expr->kids[0]; \
    expr->kids[0] = expr->kids[1];   \
    expr->kids[1] = t;               \
}

#define PERFORM_ARITH_CONVERSION(expr)                                 \
    expr->ty = CommonRealType(expr->kids[0]->ty, expr->kids[1]->ty);   \
    expr->kids[0] = Cast(expr->ty, expr->kids[0]);                     \
    expr->kids[1] = Cast(expr->ty, expr->kids[1]);


#define REPORT_OP_ERROR                                                 \
    Error(&expr->coord, "Invalid operands to %s", OPNames[expr->op]);   \
    expr->ty = T(INT);                                                  \
    return expr;                   


/**
 * Check if the expression expr is a modifiable-lvalue
 */
static int CanModify(AstExpression expr)
{	
	/**************************************************
		Modifiable:
			(1) left value
			(2) not const
			(3) for union/struct, there is not any const filed.
					
	 **************************************************/
	return (expr->lvalue && ! (expr->ty->qual & CONST) && 
	        (IsRecordType(expr->ty) ? ! ((RecordType)expr->ty)->hasConstFld : 1));
}

/**
 * Check if the expression expr is null constant
 */
static int IsNullConstant(AstExpression expr)
{
	return expr->op == OP_CONST && expr->val.i[0] == 0;
}

/**
 * Construct an expression to multiply offset by scale.
 */
 	/****************************************
		int arr[10];
		int * ptr = &arr[0];
		(ptr-3)
			--------------- In fact , we do  (ptr -sizeof(int)), that is , ptr -12 here.
			@offset		3"
			@scale		sizeof(int)	4
	 **************************************/
static AstExpression ScalePointerOffset(AstExpression offset, int scale)
{
	AstExpression expr;
	union value val;

	CREATE_AST_NODE(expr, Expression);

	expr->ty = offset->ty;
	expr->op = OP_MUL;
	expr->kids[0] = offset;
	val.i[1] = 0;
	val.i[0] = scale;
	expr->kids[1] = Constant(offset->coord, offset->ty, val);
	//PRINT_CUR_ASTNODE(offset);
	return FoldConstant(expr);
}

/**
 * Construct an expression to divide diff by size
 */
	/**************************************************
		int arr[10];
		int len = &arr[9] - &arr[0];		------------  9
	 **************************************************/ 
static AstExpression PointerDifference(AstExpression diff, int size)
{
	AstExpression expr;
	union value val;

	CREATE_AST_NODE(expr, Expression);

	expr->ty = diff->ty;
	expr->op = OP_DIV;
	expr->kids[0] = diff;
	val.i[1] = 0;
	val.i[0] = size;
	expr->kids[1] = Constant(diff->coord, diff->ty, val);

	return expr;
}

/**
 * Check primary expression
 */
/****************************************************************
 OPINFO(OP_ID,			  16,	"id",	  Primary,		  NOP)
 OPINFO(OP_CONST,		  16,	"const",  Primary,		  NOP)
 OPINFO(OP_STR, 		  16,	"str",	  Primary,		  NOP)
 ****************************************************************/
static AstExpression CheckPrimaryExpression(AstExpression expr)
{
	Symbol p;

	if (expr->op == OP_CONST)
		return expr;

	if (expr->op == OP_STR)
	{
		/**************************************************
		(1)
		int main(int argc,char * argv[]){
			printf("%p\n",&"12345");
			return 0;
		}
		see CheckAddressConstant(AstExpression expr);
		Because &"12345"	is legal in C, "12345" is different from int const, such as 123.
		we can't use &123, but &"123" is legal.
		(2)	see TranslatePrimaryExpression() for the reason 
			we don't set "expr->isarray = 1;"	 here.		
		(3)  in assembly code, "12345" do have names as global variable names.
			.str0:	.string	"%p"
		 **************************************************/
		//PRINT_DEBUG_INFO(("%s",((String)expr->val.p)->chs)); 
		expr->op = OP_ID;
		expr->val.p = AddString(expr->ty, expr->val.p,&expr->coord);
		expr->lvalue = 1;

		return expr;
	}

	p = LookupID(expr->val.p);
	if (p == NULL)
	{
		Error(&expr->coord, "Undeclared identifier: %s", expr->val.p);
		p = AddVariable(expr->val.p, T(INT), Level == 0 ? 0 : TK_AUTO,&expr->coord);
		expr->ty = T(INT);
		expr->lvalue = 1;
	}
	else if (p->kind == SK_TypedefName)
	{
		/************************************
			typedef int a;
			int b;
			b = a;	------------ error
		 ************************************/
		Error(&expr->coord, "Typedef name cannot be used as variable");
		expr->ty = T(INT);
	}
	else if (p->kind == SK_EnumConstant)
	{
		/************************************************
		see function CheckEnumerator()
			enum Color{	RED,GREEN,BLUE};

			RED is a SK_EnumConstant.
		
		 3.1.3.3 Enumeration constants		 
		 Syntax		 
				   enumeration-constant:
						   identifier		 
		 Semantics		 
			An identifier declared as an enumeration constant has type int.  
		 ************************************************/
		expr->op = OP_CONST;
		expr->val = p->val;
		expr->ty = T(INT);
	}
	else
	{	// ID,  or function designatr
		expr->ty = p->ty;
		expr->val.p = p;
		expr->inreg   = p->sclass == TK_REGISTER;
		// an ID is a lvalue, while a function designator not.
		expr->lvalue  = expr->ty->categ != FUNCTION;
	}

	return expr;
}

static AstExpression PromoteArgument(AstExpression arg)
{
	Type ty = Promote(arg->ty);
	//PRINT_DEBUG_INFO(("%s",TypeToString(ty)));
	return Cast(ty, arg);
}

/**
 * Check argument.
 * @param fty function type
 * @param arg argument expression
 * @param argNo the argument's number in function call
 * @param argFull if the function's argument is full
 */
static AstExpression CheckArgument(FunctionType fty, AstExpression arg, int argNo, int *argFull)
{
	Parameter param;
	int parLen = LEN(fty->sig->params);

	arg = Adjust(CheckExpression(arg), 1);

	if (fty->sig->hasProto && parLen == 0)
	{
		*argFull = 1;
		return arg;
	}

	if (argNo == parLen && ! fty->sig->hasEllipsis)
		*argFull = 1;
	
	if (! fty->sig->hasProto)
	{
		arg = PromoteArgument(arg);
		// see CheckFunctionCall(),	we never check the number of arguments when no prototype.
		*argFull = 0;
		return arg;
	}
	else if (argNo <= parLen)
	{
		param = GET_ITEM(fty->sig->params, argNo - 1);
		if (! CanAssign(param->ty, arg))
			goto err;

		if (param->ty->categ < INT)
			arg = Cast(T(INT), arg);
		else
			arg = Cast(param->ty, arg);

		return arg;
	}
	else
	{
		/************************************************
		The ellipsis notation in a function prototype declarator causes argument
		type conversion to stop after the last declared parameter.  The
		default argument promotions are performed on trailing arguments.
		 ************************************************/
		return PromoteArgument(arg);
	}

err:
	Error(&arg->coord, "Incompatible argument");
	return arg;
}

static AstExpression CheckFunctionCall(AstExpression expr)
{
	AstExpression arg;
	Type ty;
	AstNode *tail;
	int argNo, argFull;
	if (expr->kids[0]->op == OP_ID && LookupID(expr->kids[0]->val.p) == NULL)
	{
		expr->kids[0]->ty = DefaultFunctionType;
		expr->kids[0]->val.p = AddFunction(expr->kids[0]->val.p, DefaultFunctionType, TK_EXTERN,&expr->coord);
	}
	else
	{
		// PRINT_DEBUG_INFO(("%s",OPNames[expr->kids[0]->op]));
		expr->kids[0] = CheckExpression(expr->kids[0]);
	}
	expr->kids[0] = Adjust(expr->kids[0], 1);
	ty = expr->kids[0]->ty;

	if (! (IsPtrType(ty) && IsFunctionType(ty->bty)))
	{
		Error(&expr->coord, "The left operand must be function or function pointer");
		ty = DefaultFunctionType;
	}
	else
	{
		ty = ty->bty;
	}

	tail = (AstNode *)&expr->kids[1];
	arg = expr->kids[1];
	argNo = 1;
	argFull = 0;
	while (arg != NULL && ! argFull)
	{
		*tail = (AstNode)CheckArgument((FunctionType)ty, arg, argNo, &argFull);
		tail = &(*tail)->next;
		arg = (AstExpression)arg->next;
		argNo++;
	}
	*tail = NULL;
	
	// see examples/argument/argCount.c
	while (arg != NULL){
		CheckExpression(arg);
		arg = (AstExpression)arg->next;	
	}
	argNo--;		//	when argNo is 1, it means that we want to check argument 1, indexing from 1 to n .	
	
	/******************************************************
		the number and types of arguments are not compared with those of the
		parameters in a function definition that does not include a function
		prototype declarator.
	 ******************************************************/
	if(argNo > LEN(((FunctionType)ty)->sig->params)){
		if(((FunctionType)ty)->sig->hasProto){
			if(!((FunctionType)ty)->sig->hasEllipsis){
				Error(&expr->coord, "Too many arguments");
			}
		}else{
			Warning(&expr->coord, "Too many arguments");
		}
	}else if (argNo < LEN(((FunctionType)ty)->sig->params)){
		if(((FunctionType)ty)->sig->hasProto){
			Error(&expr->coord, "Too few arguments");
		}else{
			Warning(&expr->coord, "Too few arguments");
		}
	}
	
	expr->ty = ty->bty;

	return expr;
}

static AstExpression CheckMemberAccess(AstExpression expr)
{
	Type ty;
	Field fld;

	expr->kids[0] = CheckExpression(expr->kids[0]);
	if (expr->op == OP_MEMBER)
	{
		/***************************************
			typedef struct{
				int a;
				int b;
			}Data;
			Data dt;
			dt.a = 3;			// legal		lvalue is 1
			GetData().a = 3;	// illegal		lvalue is 0
		 ***************************************/
		expr->kids[0] = Adjust(expr->kids[0], 0);
		ty = expr->kids[0]->ty;
		if (! IsRecordType(ty))
		{
			REPORT_OP_ERROR;
		}
		expr->lvalue = expr->kids[0]->lvalue;
	}
	else
	{
		/**********************************************
		For example:
			Type ty;
			((FunctionType) ty)->sig;			// the whole expression, lvalue is 1
			((FunctionType) ty) is considered as a right value.	//lvalue is 0
		 **********************************************/
		expr->kids[0] = Adjust(expr->kids[0], 1);
		ty = expr->kids[0]->ty;
		if (! (IsPtrType(ty) && IsRecordType(ty->bty)))
		{
			REPORT_OP_ERROR;
		}
		ty = ty->bty;
		expr->lvalue = 1;
	}
	
	fld = LookupField(Unqual(ty), expr->val.p);
	if (fld == NULL)
	{
		Error(&expr->coord, "struct or union member %s doesn't exsist", expr->val.p);
		expr->ty = T(INT);
		return expr;
	}	
	expr->ty = Qualify(ty->qual, fld->ty);	
	expr->val.p = fld;
	expr->bitfld = fld->bits != 0;
	return expr;
}
//	a++,a--,++a,--a		------------->     a+= 1, a-=1
static AstExpression TransformIncrement(AstExpression expr)
{
	AstExpression casgn;
	union value val;
	
	val.i[1] = 0; val.i[0] = 1;
	CREATE_AST_NODE(casgn, Expression);
	casgn->coord = expr->coord;
	//	a++, ++a -------------->  a += 1
	//	a--, --a	 -------------- >  a -= 1
	casgn->op = (expr->op == OP_POSTINC || expr->op == OP_PREINC) ? OP_ADD_ASSIGN : OP_SUB_ASSIGN;
	casgn->kids[0] = expr->kids[0];
	casgn->kids[1] = Constant(expr->coord, T(INT), val);

	expr->kids[0] = CheckExpression(casgn);
	expr->ty = expr->kids[0]->ty;
	return expr;
}
/**********************************************************************
	 OPINFO(OP_INDEX,		  15,	"[]",	  Postfix,		  NOP)
	 OPINFO(OP_CALL,		  15,	"call",   Postfix,		  NOP)
	 OPINFO(OP_MEMBER,		  15,	".",	  Postfix,		  NOP)
	 OPINFO(OP_PTR_MEMBER,	  15,	"->",	  Postfix,		  NOP)
	 OPINFO(OP_POSTINC, 	  15,	"++",	  Postfix,		  INC)
	 OPINFO(OP_POSTDEC, 	  15,	"--",	  Postfix,		  DEC)
 **********************************************************************/
static AstExpression CheckPostfixExpression(AstExpression expr)
{
	switch (expr->op)
	{
	case OP_INDEX:
		expr->kids[0] = Adjust(CheckExpression(expr->kids[0]), 1);
		expr->kids[1] = Adjust(CheckExpression(expr->kids[1]), 1);
		if (IsIntegType(expr->kids[0]->ty))
		{
			SWAP_KIDS(expr);
		}
		if (IsObjectPtr(expr->kids[0]->ty) && IsIntegType(expr->kids[1]->ty))
		{
			expr->ty = expr->kids[0]->ty->bty;
			/****************************************************
				int arr[3][5];
				arr[1]	is considered a left value here(not accurate)
				arr[1] is an array, not a left value.
				Later, in Adjust(...), set lvalue to 0.
			 ****************************************************/
			expr->lvalue = 1;
			expr->kids[1] = DoIntegerPromotion(expr->kids[1]);
			expr->kids[1] = ScalePointerOffset(expr->kids[1], expr->ty->size);
			//  see examples/array/array.c			
			//	We use "expr->ty->categ != ARRAY" , not "!expr->isarray".
			//	Because of post-order visiting of the AST,  at this time, expr->isarray is always 0.
			if(!expr->kids[0]->isarray && expr->ty->categ != ARRAY ){
			//if(expr->ty->categ != ARRAY){
			/****************************************************
			int arr2[2][2][2];
			arr2[1][1][1];
			(1)	assume	C1	be
				if(!expr->kids[0]->isarray && expr->ty->categ != ARRAY )
			(2)	assume	C2	be
				if(expr->ty->categ != ARRAY)
			(3)	C1 and C2 both are right here.
				But if we use C2:
					arr2[1][1][1] are accessed by:
					leal arr2+24, %ecx
					addl $4, %ecx
					movl (%ecx), %edx
				But if we use C1:
					movl arr2+28,%edx
			 ****************************************************/
				AstExpression deref, addExpr;
				CREATE_AST_NODE(deref, Expression);
				CREATE_AST_NODE(addExpr, Expression);
				deref->op = OP_DEREF;
				deref->ty = expr->kids[0]->ty->bty;
				deref->kids[0] = addExpr ;
				
				addExpr->op = OP_ADD;
				addExpr->ty = expr->kids[0]->ty;
				addExpr->kids[0] = expr->kids[0];
				addExpr->kids[1] = expr->kids[1];
				//PRINT_I_AM_HERE();
				deref->lvalue = 1;
				return deref;
			}
			return expr;
		}

		REPORT_OP_ERROR;

	case OP_CALL:
		return CheckFunctionCall(expr);

	case OP_MEMBER:
	case OP_PTR_MEMBER:
		return CheckMemberAccess(expr);

	case OP_POSTINC:		// a++
	case OP_POSTDEC:	// a--
		return TransformIncrement(expr);

	default:
		assert(0);
	}

	REPORT_OP_ERROR;
}

/************************************************************
	 struct Data1{
		 int a;
	 };
	 struct Data2{
		 double f;
	 };
	 int main(){
		 struct Data1 d1;
		 struct Data2 d2;
		 d2 = (struct Data1)d1;	------------------  error, not 
		 	//	d1's type is struct , not scalar
		 	// 	(struct Data1) is struct, not scalar
		 d2 = *((struct Data2 *)&d1);--------------- ok	
		 	//	&d1's type is pointer, a scalar
		 	//  (struct Data2 *) is also a pointer.
	}
 ************************************************************/
static AstExpression CheckTypeCast(AstExpression expr)
{
	Type ty;
	// see ParseUnaryExpression() for the astExpression node of OP_CAST after syntax parsing.
	ty = CheckTypeName((AstTypeName)expr->kids[0]);
	//	
	expr->kids[1] = Adjust(CheckExpression(expr->kids[1]), 1);
	/**************************************************************
	see	asni.c.txt
		   Unless the type name specifies void type, the type name shall
	specify qualified or unqualified scalar type and the operand shall
	have scalar type.
	 **************************************************************/
	if (! (BothScalarType(ty, expr->kids[1]->ty) || ty->categ == VOID))
	{
		Error(&expr->coord, "Illegal type cast");
		return expr->kids[1];
	}
	// BothScalarType(ty, expr->kids[1]->ty) || ty->categ == VOID
	return Cast(ty, expr->kids[1]);
}

/************************************************************
 Syntax
 
		   unary-expression:
				   postfix-expression
				   ++  unary-expression
				   --  unary-expression
				   unary-operator cast-expression
				   sizeof  unary-expression
				   sizeof (  type-name )
 
		   unary-operator: one of
				   &  *  +	-  ~  !
				   
 OPINFO(OP_CAST,		  14,	"cast",   Unary,		  NOP)
 OPINFO(OP_PREINC,		  14,	"++",	  Unary,		  NOP)
 OPINFO(OP_PREDEC,		  14,	"--",	  Unary,		  NOP)
 OPINFO(OP_ADDRESS, 	  14,	"&",	  Unary,		  ADDR)
 OPINFO(OP_DEREF,		  14,	"*",	  Unary,		  DEREF)
 OPINFO(OP_POS, 		  14,	"+",	  Unary,		  NOP)
 OPINFO(OP_NEG, 		  14,	"-",	  Unary,		  NEG)
 OPINFO(OP_COMP,		  14,	"~",	  Unary,		  BCOM)
 OPINFO(OP_NOT, 		  14,	"!",	  Unary,		  NOP)
 OPINFO(OP_SIZEOF,		  14,	"sizeof", Unary,		  NOP)
 ************************************************************/
static AstExpression CheckUnaryExpression(AstExpression expr)
{
	Type ty;

	switch (expr->op)
	{
	case OP_PREINC:	// ++a
	case OP_PREDEC:	// --a
		// PRINT_I_AM_HERE();
		return TransformIncrement(expr);

	case OP_ADDRESS:	// &a
		expr->kids[0] = CheckExpression(expr->kids[0]);
		ty = expr->kids[0]->ty;
		//PRINT_DEBUG_INFO(("%s",OPNames[expr->kids[0]->op]));
		if (expr->kids[0]->op == OP_DEREF)
		{
			/****************************************************
			//	&*a;	------------>  a		('a' is pointer,  not lvalue)
				int a = 30;
				int * ptr = &a;
				int * ptr2 = &*ptr;
				&*ptr = &a;	----------> error	, &*ptr not a lvalue.
			 ****************************************************/
			expr->kids[0]->kids[0]->lvalue = 0;
			return expr->kids[0]->kids[0];
		}
		else if (expr->kids[0]->op == OP_INDEX)
		{
			/*******************************************************
				int a[3];
				&a[3];	---------->   pointer arithmetic operation
							Pointer(INT)	+  3
			 *******************************************************/
			expr->kids[0]->op = OP_ADD;
			expr->kids[0]->ty = PointerTo(ty);
			expr->kids[0]->lvalue = 0;
			return expr->kids[0];
		}
		else if (IsFunctionType(ty) || 
			     (expr->kids[0]->lvalue && ! expr->kids[0]->bitfld && ! expr->kids[0]->inreg))
		{
			/*****************************************************************
				see ansi.c.txt
				   The operand of the unary & operator shall be either a function
				designator or an lvalue that designates an object that is not a
				bit-field and is not declared with the register storage-class
				specifier.
				void f(){
				}
				int main(){
					&f;
					//&&f;	----------------- error, &f is a pointer to function, not function designator
					return 0;
				}
			 ******************************************************************/
			// to make it clear taht "&a" is not a lvalue.
			expr->lvalue = 0;
			expr->ty = PointerTo(ty);
			//PRINT_I_AM_HERE();
			return expr;
		}
		break;

	case OP_DEREF:	// *
		//PRINT_I_AM_HERE();
		expr->kids[0] = Adjust(CheckExpression(expr->kids[0]), 1);
		ty = expr->kids[0]->ty;
		if (expr->kids[0]->op == OP_ADDRESS)
		{
			/*********************************************
				int a = 3;
				*&a = 5;		----------- *(&a) is a lvalue, so it is OK.
				printf("%d \n",a);

				*&a ---------------->  a
			 *********************************************/
			expr->kids[0]->kids[0]->ty = ty->bty;
			return expr->kids[0]->kids[0];
		}
#if 0		// commented	
		else if (expr->kids[0]->op == OP_ADD && expr->kids[0]->kids[0]->isarray)
#endif	
#if 1
		else if (expr->kids[0]->op == OP_ADD &&
			(ty->bty->categ == ARRAY|| expr->kids[0]->kids[0]->isarray))
#endif
		{
			/**********************************************
				int arr[3];
				*(arr+3)	--------->  arr[3]
						OP_DEREF--> OP_ADD	 ----->	arr
											 ----->	3

						is converted to 
						
						OP_INDEX ----------> arr
								 ----------->3
			 **********************************************/
			expr->kids[0]->op = OP_INDEX;
			expr->kids[0]->ty = ty->bty;
			expr->kids[0]->lvalue = 1;
			return expr->kids[0];
		}
		if (IsPtrType(ty))
		{
			// The operand of the unary * operator shall have pointer type. 
			/****************************************
				We know the type of the @expr now.
				int number = 3;
				int * ptr = &number;
				*ptr = 5;

				struct astExpression
					op:OP_DEREF
					ty	------------		we can fill this filed now.	It's T(INT)
					kids[0]	---------------  struct astExpression
											OP_ID
											"ptr"
											ty---------> POINTER(INT)
			 ****************************************/
			expr->ty = ty->bty;
			/******************************************
			 void f(void){
				 printf("void f(void)\n");
			 }
			 int main(int argc,char * argv[]){
				 (*f)(); 
				 return 0;
			 }
			 ******************************************/
			if (IsFunctionType(expr->ty))
			{
				return expr->kids[0];
			}
			// see examples/array/offset.c
			if(expr->ty->categ == ARRAY || expr->kids[0]->isarray){
				union value val;
				val.i[0] = val.i[1] = 0;
				expr->kids[1] = Constant(expr->coord, T(INT), val);
				expr->op = OP_INDEX;
			}
			/*******************************
				int num = 3;
				int * ptr1 = &num;
				*ptr1 = 5;
				int ** ptr2 = &ptr1;

				*ptr2 = &num;	-------------> Even  *ptr2  is still a lvalue.

			 ********************************/
			expr->lvalue = 1;
			return expr;
		}
		break;

	case OP_POS:		// +a	-----------  rvalue, not lvalue
	case OP_NEG:	// -a
		expr->kids[0] = Adjust(CheckExpression(expr->kids[0]), 1);
		/***********************************************************
		The operand of the unary + or - operator shall have arithmetic
		type
		 ***********************************************************/
		if (IsArithType(expr->kids[0]->ty))
		{
			/***************************************************
				The result of the unary + operator is the value of its operand.
			 The integral promotion is performed on the operand, and the result has
			 the promoted type.
			 
				The result of the unary - operator is the negative of its operand.
			 The integral promotion is performed on the operand, and the result has
			 the promoted type.
			 ***************************************************/
			expr->kids[0] = DoIntegerPromotion(expr->kids[0]);
			expr->ty = expr->kids[0]->ty;
			//	+a  is still a;
			//   for -a, we do constant-folding.
			return expr->op == OP_POS ? expr->kids[0] : FoldConstant(expr);
		}
		break;

	case OP_COMP:	// ~a
		expr->kids[0] = Adjust(CheckExpression(expr->kids[0]), 1);
		// The operand of the ~ operator, integral type; 
		if (IsIntegType(expr->kids[0]->ty))
		{
			expr->kids[0] = DoIntegerPromotion(expr->kids[0]);
			expr->ty = expr->kids[0]->ty;
			return FoldConstant(expr);
		}
		break;

	case OP_NOT:	// !a
		expr->kids[0] = Adjust(CheckExpression(expr->kids[0]), 1);
		if (IsScalarType(expr->kids[0]->ty))
		{
			expr->ty = T(INT);
			return FoldConstant(expr);
		}
		break;

	case OP_SIZEOF:	// sizeof(a)
		/***************************************
		     sizeof  unary-expression
                  sizeof (  type-name )
		 ***************************************/
		if (expr->kids[0]->kind == NK_Expression)
		{
			expr->kids[0] = CheckExpression(expr->kids[0]);
			// sizeof can't be applied to a bit-field
			if (expr->kids[0]->bitfld)
				goto err;
			ty = expr->kids[0]->ty;
		}
		else
		{
			ty = CheckTypeName((AstTypeName)expr->kids[0]);
		}
		/*****************************************************
		   The sizeof operator shall not be applied to an expression that has
		function type or an incomplete type, to the parenthesized name of such
		a type, or to an lvalue that designates a bit-field object.

		The keypoint is :
			what is the meaning of "an imcomplete type"?
		 *****************************************************/
		if (IsFunctionType(ty) || IsIncompleteType(ty,!IGNORE_ZERO_SIZE_ARRAY)){
			goto err;
		}
		/**********************************************
		   The sizeof operator yields the size (in bytes) of its operand,
		which may be an expression or the parenthesized name of a type.  The
		size is determined from the type of the operand, which is not itself
		evaluated.  The result is an integer constant.
		 **********************************************/
		expr->ty = T(UINT);
		expr->op = OP_CONST;
		expr->val.i[0] = ty->size;
		return expr;

	case OP_CAST:	// (int)a
		return CheckTypeCast(expr);

	default:
		assert(0);
	}

err:
	REPORT_OP_ERROR;

}
/******************************************************************************
 Syntax
		   multiplicative-expression:
				   cast-expression
				   multiplicative-expression *	cast-expression
				   multiplicative-expression /	cast-expression
				   multiplicative-expression %	cast-expression 
 Constraints
 
	Each of the operands shall have arithmetic type.  The operands of
 the % operator shall have integral type.

 ******************************************************************************/
static AstExpression CheckMultiplicativeOP(AstExpression expr)
{
	if (expr->op != OP_MOD && BothArithType(expr->kids[0]->ty, expr->kids[1]->ty))
		goto ok;

	if (expr->op == OP_MOD && BothIntegType(expr->kids[0]->ty, expr->kids[1]->ty))
		goto ok;

	REPORT_OP_ERROR;

ok:
	PERFORM_ARITH_CONVERSION(expr);
	return FoldConstant(expr);
}
/********************************************************************
 Syntax 
		   additive-expression:
				   multiplicative-expression
				   additive-expression +  multiplicative-expression
				   additive-expression -  multiplicative-expression 
 Constraints 
	For addition, either both operands shall have arithmetic type, or
 one operand shall be a pointer to an object type and the other shall
 have integral type.  (Incrementing is equivalent to adding 1.)

 ********************************************************************/
static AstExpression CheckAddOP(AstExpression expr)
{
	Type ty1, ty2;
	//PRINT_I_AM_HERE();
	if (expr->kids[0]->op == OP_CONST)
	{
		SWAP_KIDS(expr);
	}
	ty1 = expr->kids[0]->ty;
	ty2 = expr->kids[1]->ty;
	//	3+4
	if (BothArithType(ty1, ty2))
	{
		PERFORM_ARITH_CONVERSION(expr);
		return FoldConstant(expr);
	}
	if (IsObjectPtr(ty2) && IsIntegType(ty1))
	{
		SWAP_KIDS(expr);
		ty1 = expr->kids[0]->ty;
		goto left_ptr;
	}

	if (IsObjectPtr(ty1) && IsIntegType(ty2))
	{
left_ptr:
		/****************************************
			int a = 3;
			int * ptr = &a;
			ptr+5;	------------>  In fact , we shall do ptr + 5 * sizeof(int)

			Here, expr->kids[1] is 5
				 ty1 is the type of ptr, that is , POINTER(INT)
		 ****************************************/	
		expr->kids[1] = DoIntegerPromotion(expr->kids[1]);
		expr->kids[1] = ScalePointerOffset(expr->kids[1], ty1->bty->size);
		expr->ty = ty1;
		return expr;
	}

	REPORT_OP_ERROR;
}
/************************************************************
   For subtraction, one of the following shall hold: 

 * both operands have arithmetic type; 

 * both operands are pointers to qualified or unqualified versions of
   compatible object types; or

 * the left operand is a pointer to an object type and the right
   operand has integral type.  (Decrementing is equivalent to subtracting 1.)

 ************************************************************/
static AstExpression CheckSubOP(AstExpression expr)
{
	Type ty1, ty2;

	ty1 = expr->kids[0]->ty;
	ty2 = expr->kids[1]->ty;
	// 3 + 4
	if (BothArithType(ty1, ty2))
	{
		PERFORM_ARITH_CONVERSION(expr);
		return FoldConstant(expr);
	}
	/****************************************
		int arr[10];
		int * ptr = &arr[0];
		(ptr-3)
			--------------- In fact , we do  (ptr -sizeof(int)), that is , ptr -12 here.
	 *****************************************/
	if (IsObjectPtr(ty1) && IsIntegType(ty2))
	{
		expr->kids[1] = DoIntegerPromotion(expr->kids[1]);
		expr->kids[1] = ScalePointerOffset(expr->kids[1], ty1->bty->size);
		expr->ty = ty1;
		return expr;
	}
	/**************************************************
		int arr[10];
		int len = &arr[9] - &arr[0];		------------  9
	 **************************************************/
	if (IsCompatiblePtr(ty1, ty2))
	{
		expr->ty = T(INT);
		expr = PointerDifference(expr, ty1->bty->size);
		return expr;
	}

	REPORT_OP_ERROR;
}
/**************************************************************
 Syntax 
		   shift-expression:
				   additive-expression
				   shift-expression <<	additive-expression
				   shift-expression >>	additive-expression
 
 Constraints
 
	Each of the operands shall have integral type.
	The integral promotions are performed on each of the operands.  The
type of the result is that of the promoted left operand.
 **************************************************************/
static AstExpression CheckShiftOP(AstExpression expr)
{
	if (BothIntegType(expr->kids[0]->ty, expr->kids[1]->ty))
	{
		expr->kids[0] = DoIntegerPromotion(expr->kids[0]);
		expr->kids[1] = DoIntegerPromotion(expr->kids[1]);
		expr->ty = expr->kids[0]->ty;

		return FoldConstant(expr);
	}

	REPORT_OP_ERROR;
}
/***********************************************************************
 relational-expression:
		 shift-expression
		 relational-expression <   shift-expression
		 relational-expression >   shift-expression
		 relational-expression <=  shift-expression
		 relational-expression >=  shift-expression
 ***********************************************************************/
static AstExpression CheckRelationalOP(AstExpression expr)
{
	Type ty1, ty2;
	
	expr->ty = T(INT);
	ty1 = expr->kids[0]->ty;
	ty2 = expr->kids[1]->ty;
	//	 both operands have arithmetic type; 
	if (BothArithType(ty1, ty2))
	{
		PERFORM_ARITH_CONVERSION(expr);
		expr->ty = T(INT);
		return FoldConstant(expr);
	}
	/******************************************************
		both operands are pointers to qualified or unqualified versions of
	   compatible object types; 
	 ******************************************************/
	if (IsObjectPtr(ty1) && IsObjectPtr(ty2) && 
		IsCompatibleType(Unqual(ty1->bty), Unqual(ty2->bty)))
	{
		return expr;
	}
	/*******************************************************
		both operands are pointers to qualified or unqualified versions of
	   compatible incomplete types.
	 *******************************************************/
	if (IsIncompletePtr(ty1) && IsIncompletePtr(ty2) &&
		IsCompatibleType(Unqual(ty1->bty), Unqual(ty2->bty)))
	{
		return expr;
	}

	REPORT_OP_ERROR;
}
/****************************************************************
 Syntax
 
		   equality-expression:
				   relational-expression
				   equality-expression ==  relational-expression
				   equality-expression !=  relational-expression 

 ****************************************************************/
static AstExpression CheckEqualityOP(AstExpression expr)
{
	Type ty1, ty2;

	expr->ty = T(INT);
	ty1 = expr->kids[0]->ty;
	ty2 = expr->kids[1]->ty;
	// both operands have arithmetic type;
	if (BothArithType(ty1, ty2))
	{
		PERFORM_ARITH_CONVERSION(expr);
		expr->ty = T(INT);
		return FoldConstant(expr);
	}
	/************************************************
	(1) both operands are pointers to qualified or unqualified versions of
	compatible types;
	
#define IsCompatiblePtr(ty1, ty2) (IsPtrType(ty1) && IsPtrType(ty2) &&  \
                                   IsCompatibleType(Unqual(ty1->bty), Unqual(ty2->bty)))		
	(2) one operand is a pointer to an object or incomplete type and the
	other is a qualified or unqualified version of void ;
	(3)one operand is a pointer and the other is a null pointer constant.
	 ***************************************************/
	if (IsCompatiblePtr(ty1, ty2) ||
		NotFunctionPtr(ty1) && IsVoidPtr(ty2) ||
		NotFunctionPtr(ty2) && IsVoidPtr(ty1) ||
		IsPtrType(ty1) && IsNullConstant(expr->kids[1]) ||
		IsPtrType(ty2) && IsNullConstant(expr->kids[0]))
	{
		return expr;
	}

	REPORT_OP_ERROR;
}
/************************************************************
 Syntax 
		   AND-expression:
				   equality-expression
				   AND-expression &  equality-expression 
				   
          exclusive-OR-expression:
                  AND-expression
                  exclusive-OR-expression ^  AND-expression		

          inclusive-OR-expression:
                  exclusive-OR-expression
                  inclusive-OR-expression |  exclusive-OR-expression                  
 Constraints 
	Each of the operands shall have integral type.	
 ************************************************************/
static AstExpression CheckBitwiseOP(AstExpression expr)
{
	if (BothIntegType(expr->kids[0]->ty, expr->kids[1]->ty))
	{
		PERFORM_ARITH_CONVERSION(expr);
		return FoldConstant(expr);
	}

	REPORT_OP_ERROR;
}
/*****************************************************************
 Syntax
 
		   logical-AND-expression:
				   inclusive-OR-expression
				   logical-AND-expression &&  inclusive-OR-expression
				   
          logical-OR-expression:
                  logical-AND-expression
                  logical-OR-expression ||  logical-AND-expression				   
 
 Constraints
 
	Each of the operands shall have scalar type. 
	The result has type int.
 *****************************************************************/
static AstExpression CheckLogicalOP(AstExpression expr)
{
	if (BothScalarType(expr->kids[0]->ty, expr->kids[1]->ty))
	{
		expr->ty = T(INT);
		return FoldConstant(expr);
	}

	REPORT_OP_ERROR;		
}
/***************************************************************
Binary opeator:
	 OPINFO(OP_OR,			  4,	"||",	  Binary,		  NOP)
	 OPINFO(OP_AND, 		  5,	"&&",	  Binary,		  NOP)
	 OPINFO(OP_BITOR,		  6,	"|",	  Binary,		  BOR)
	 OPINFO(OP_BITXOR,		  7,	"^",	  Binary,		  BXOR)
	 OPINFO(OP_BITAND,		  8,	"&",	  Binary,		  BAND)
	 OPINFO(OP_EQUAL,		  9,	"==",	  Binary,		  JE)
	 OPINFO(OP_UNEQUAL, 	  9,	"!=",	  Binary,		  JNE)
	 OPINFO(OP_GREAT,		  10,	">",	  Binary,		  JG)
	 OPINFO(OP_LESS,		  10,	"<",	  Binary,		  JL)
	 OPINFO(OP_GREAT_EQ,	  10,	">=",	  Binary,		  JGE)
	 OPINFO(OP_LESS_EQ, 	  10,	"<=",	  Binary,		  JLE)
	 OPINFO(OP_LSHIFT,		  11,	"<<",	  Binary,		  LSH)
	 OPINFO(OP_RSHIFT,		  11,	">>",	  Binary,		  RSH)
	 OPINFO(OP_ADD, 		  12,	"+",	  Binary,		  ADD)
	 OPINFO(OP_SUB, 		  12,	"-",	  Binary,		  SUB)
	 OPINFO(OP_MUL, 		  13,	"*",	  Binary,		  MUL)
	 OPINFO(OP_DIV, 		  13,	"/",	  Binary,		  DIV)
	 OPINFO(OP_MOD, 		  13,	"%",	  Binary,		  MOD)

 ***************************************************************/
static AstExpression (* BinaryOPCheckers[])(AstExpression) = 
{
	CheckLogicalOP,	// "||"
	CheckLogicalOP,	// "&&"
	CheckBitwiseOP,	// |
	CheckBitwiseOP, // ^
	CheckBitwiseOP,	// &
	CheckEqualityOP,	// ==
	CheckEqualityOP,	// !=
	CheckRelationalOP,	// >
	CheckRelationalOP,	// <
	CheckRelationalOP,	// >=
	CheckRelationalOP,	// <=
	CheckShiftOP,		//	<<
	CheckShiftOP,		// >>
	CheckAddOP,			// +
	CheckSubOP,			// -
	CheckMultiplicativeOP,	// *
	CheckMultiplicativeOP,	//  /
	CheckMultiplicativeOP	// %
};
/************************************************************
// This function is used later in 
 static AstExpression (* ExprCheckers[])(AstExpression) = {
 }
 ************************************************************/
static AstExpression CheckBinaryExpression(AstExpression expr)
{

	expr->kids[0] = Adjust(CheckExpression(expr->kids[0]), 1);;
	expr->kids[1] = Adjust(CheckExpression(expr->kids[1]), 1);
	return (* BinaryOPCheckers[expr->op - OP_OR])(expr);
}
/*********************************************************************
 Syntax
 
		   assignment-expression:
				   conditional-expression
				   unary-expression assignment-operator assignment-expression
 
		   assignment-operator: one of
				   =  *=  /=  %=  +=  -=  <<=  >>=	&=	^=	|=s

OPINFO(OP_ASSIGN,        2,    "=",      Assignment,     NOP)
OPINFO(OP_BITOR_ASSIGN,  2,    "|=",     Assignment,     NOP)
OPINFO(OP_BITXOR_ASSIGN, 2,    "^=",     Assignment,     NOP)
OPINFO(OP_BITAND_ASSIGN, 2,    "&=",     Assignment,     NOP)
OPINFO(OP_LSHIFT_ASSIGN, 2,    "<<=",    Assignment,     NOP)
OPINFO(OP_RSHIFT_ASSIGN, 2,    ">>=",    Assignment,     NOP)
OPINFO(OP_ADD_ASSIGN,    2,    "+=",     Assignment,     NOP)
OPINFO(OP_SUB_ASSIGN,    2,    "-=",     Assignment,     NOP)
OPINFO(OP_MUL_ASSIGN,    2,    "*=",     Assignment,     NOP)
OPINFO(OP_DIV_ASSIGN,    2,    "/=",     Assignment,     NOP)
OPINFO(OP_MOD_ASSIGN,    2,    "%=",     Assignment,     NOP)				   
 *********************************************************************/
static AstExpression CheckAssignmentExpression(AstExpression expr)
{
	int ops[] = 
	{ 
		OP_BITOR, OP_BITXOR, OP_BITAND, OP_LSHIFT, OP_RSHIFT, 
		OP_ADD,	  OP_SUB,    OP_MUL,    OP_DIV,    OP_MOD 
	};
	Type ty;
	
	expr->kids[0] = Adjust(CheckExpression(expr->kids[0]), 0);
	expr->kids[1] = Adjust(CheckExpression(expr->kids[1]), 1);
	/**************************************************************
	   An assignment operator shall have a modifiable lvalue as its left operand. 
	 **************************************************************/
	if (! CanModify(expr->kids[0]))
	{
		Error(&expr->coord, "The left operand cannot be modified");
	}
	/************************************************************
	//	a += b  -------------->   a = a + b;
	see 3.3.16.2 Compound assignment
		
		   A compound assignment of the form E1 op = E2 differs from the
	simple assignment expression E1 = E1 op (E2) only in that the lvalue
	E1 is evaluated only once.
	 ************************************************************/
	if (expr->op != OP_ASSIGN)
	{		
		AstExpression lopr;

		CREATE_AST_NODE(lopr, Expression);
		lopr->coord   = expr->coord;
		lopr->op      = ops[expr->op - OP_BITOR_ASSIGN];
		lopr->kids[0] = expr->kids[0];
		lopr->kids[1] = expr->kids[1];
		/******************************************************
			Look out !
			We don't call CheckBinaryExpression() here.
			That is ,we don't touch the lvalue here.
		 ******************************************************/
		//PRINT_DEBUG_INFO(("lvalue = %d",expr->kids[0]->lvalue));
		expr->kids[1] = (*BinaryOPCheckers[lopr->op - OP_OR])(lopr);
		//PRINT_DEBUG_INFO(("lvalue = %d",expr->kids[1]->kids[0]->lvalue));
	}
	// we have use CanModify() to test whether left operand is modifiable.
	ty = expr->kids[0]->ty;
	
	// we are sure ty is not qualified by CONST now.
	if (! CanAssign(ty, expr->kids[1]))
	{
		Error(&expr->coord, "Wrong assignment");
	}
	else
	{
		expr->kids[1] = Cast(ty, expr->kids[1]);
	}
	expr->ty = ty;
	return expr;
}
/************************************************************************
 Syntax
 
		   conditional-expression:
				   logical-OR-expression
				   logical-OR-expression ?	expression :  conditional-expression

 OPINFO(OP_QUESTION,	  3,	"?",	  Conditional,	  NOP)

 *************************************************************************/
static AstExpression CheckConditionalExpression(AstExpression expr)
{
	int qual;
	Type ty1, ty2;
	/******************************************************************
		the first operand ?  the second operand: the third operand
	 ******************************************************************/
	// the first operand
	expr->kids[0] = Adjust(CheckExpression(expr->kids[0]), 1);
	// The first operand shall have scalar type.
	if (! IsScalarType(expr->kids[0]->ty))
	{
		Error(&expr->coord, "The first expression shall be scalar type.");
	}
	// the second operand
	expr->kids[1]->kids[0] = Adjust(CheckExpression(expr->kids[1]->kids[0]), 1);
	// the third operand
	expr->kids[1]->kids[1] = Adjust(CheckExpression(expr->kids[1]->kids[1]), 1);

	ty1 = expr->kids[1]->kids[0]->ty;
	ty2 = expr->kids[1]->kids[1]->ty;
	// both operands have arithmetic type; 
	if (BothArithType(ty1, ty2))
	{
		/*************************************************************
				 If both the second and third operands have arithmetic type, the
		usual arithmetic conversions are performed to bring them to a common
		type and the result has that type. 
		 *************************************************************/
		expr->ty = CommonRealType(ty1, ty2);
		expr->kids[1]->kids[0] = Cast(expr->ty, expr->kids[1]->kids[0]);
		expr->kids[1]->kids[1] = Cast(expr->ty, expr->kids[1]->kids[1]);

		return FoldConstant(expr);
	}
	else if (IsRecordType(ty1) && ty1 == ty2)
	{
		/************************************************
		both operands have compatible structure or union types; 
		Here, 
			ty1 == ty2 is the test of compatible strucure / union

		If both the operands have
		structure or union type, the result has that type.			
		 ************************************************/
		expr->ty = ty1;
	}
	else if (ty1->categ == VOID && ty2->categ == VOID)
	{	// both operands have void type;  the result has void type.
		expr->ty = T(VOID);
	}
	/**************************************************************
			   If both the second and third operands are pointers or one is a null
		pointer constant and the other is a pointer, the result type is a
		pointer to a type qualified with all the type qualifiers of the types
		pointed-to by both operands.  			
	 **************************************************************/
	else if (IsCompatiblePtr(ty1, ty2))
	{
		/********************************************
			both operands are pointers to qualified or unqualified versions of
		compatible types;
		 ********************************************/
		qual = ty1->bty->qual | ty2->bty->qual;
		/*********************************************************
		Furthermore, if both operands are
		pointers to compatible types or differently qualified versions of a
		compatible type, the result has the composite type;
		 *********************************************************/
		expr->ty  = PointerTo(Qualify(qual, CompositeType(Unqual(ty1->bty), Unqual(ty2->bty))));
	}
	/************************************************************
		if one operand is
		a null pointer constant, the result has the type of the other operand;
	 ************************************************************/
	else if (IsPtrType(ty1) && IsNullConstant(expr->kids[1]->kids[1]))
	{
		// one operand is a pointer and the other is a null pointer constant; 
		expr->ty = ty1;
	}
	else if (IsPtrType(ty2) && IsNullConstant(expr->kids[1]->kids[0]))
	{
		//  one operand is a pointer and the other is a null pointer constant; 
		expr->ty = ty2;
	}
	/*********************************************************************
		otherwise, one operand is a pointer to void or a qualified version of
		void, in which case the other operand is converted to type pointer to
		void, and the result has that type.
	 ********************************************************************/
	else if (NotFunctionPtr(ty1) && IsVoidPtr(ty2) ||
	         NotFunctionPtr(ty2) && IsVoidPtr(ty1))
	{
		/**************************************************************
		 one operand is a pointer to an object or incomplete type and the
			 other is a pointer to a qualified or unqualified version of void .
		 ***************************************************************/
		qual = ty1->bty->qual | ty2->bty->qual;
		expr->ty = PointerTo(Qualify(qual, T(VOID)));
	}
	else
	{
		Error(&expr->coord, "invalid operand for ? operator.");
		expr->ty = T(INT);
	}

	return expr;
}
/******************************************************************
 Syntax
 
		   expression:
				   assignment-expression
				   expression ,  assignment-expression
 
 Semantics
 
	The left operand of a comma operator is evaluated as a void
 expression; there is a sequence point after its evaluation.  Then the
 right operand is evaluated; the result has its type and value.

 OPINFO(OP_COMMA,		  1,	",",	  Comma,		  NOP)
 ******************************************************************/
static AstExpression CheckCommaExpression(AstExpression expr)
{
	expr->kids[0] = Adjust(CheckExpression(expr->kids[0]), 1);
	expr->kids[1] = Adjust(CheckExpression(expr->kids[1]), 1);

	expr->ty = expr->kids[1]->ty;

	return expr;
}

static AstExpression CheckErrorExpression(AstExpression expr)
{
	assert(0);
	return NULL;
}

static AstExpression (* ExprCheckers[])(AstExpression) = 
{
#define OPINFO(op, prec, name, func, opcode) Check##func##Expression,
#include "opinfo.h"
#undef OPINFO
};
/************************************************************
	Create a AstExpression Node whose op is OP_CAST.
 ************************************************************/
static AstExpression CastExpression(Type ty, AstExpression expr)
{
	AstExpression cast;
	// (float)3 ----------->  3.0f
	//	int a = 3;	(void) a;		------------ legal.
	//PRINT_DEBUG_INFO(("CastExpression(): %s",GetCategName(expr->ty->categ)));
	if (expr->op == OP_CONST && ty->categ != VOID)
		return FoldCast(ty, expr);

	CREATE_AST_NODE(cast, Expression);

	cast->coord = expr->coord;
	cast->op = OP_CAST;
	cast->ty = ty;
	cast->kids[0] = expr;
	
	return cast;
}
/**************************************************************************
//	Judge whether "a = b;" is legal.
//	When this function is called, we are sure ty is not qualified by CONST now ?
	see CheckAssignmentExpression(AstExpression expr)
						we call  CanModify() before calling CanAssign()
	see 		 3.3.16.1 Simple assignment
 **************************************************************************/
int CanAssign(Type lty, AstExpression expr)
{
	Type rty = expr->ty;
	/*****************************************************
	the left operand has a qualified or unqualified version of a
	structure or union type compatible with the type of the right;
	 *****************************************************/
	lty = Unqual(lty);
	rty = Unqual(rty);
	if (lty == rty)
	{
		return 1;
	}
	/*************************************************************
	the left operand has qualified or unqualified arithmetic type and
	the right has arithmetic type;
	 *************************************************************/
	if (BothArithType(lty, rty))
	{
		return 1;
	}
	/*******************************************************************
	both operands are pointers to qualified or unqualified versions of
	compatible types, and the type pointed to by the left has all the
	qualifiers of the type pointed to by the right; 
	 ********************************************************************/
	if (IsCompatiblePtr(lty, rty) && ((lty->bty->qual & rty->bty->qual) == rty->bty->qual))
	{
		return 1;
	}
	/************************************************************************
	one operand is a pointer to an object or incomplete type and the
	other is a pointer to a qualified or unqualified version of void, 
	and
	the type pointed to by the left has all the qualifiers of the type
	pointed to by the right; or
	 *************************************************************************/
	if ((NotFunctionPtr(lty) && IsVoidPtr(rty) || NotFunctionPtr(rty) && IsVoidPtr(lty))&& 
		((lty->bty->qual & rty->bty->qual) == rty->bty->qual))
	{
		return 1;
	}
	// the left operand is a pointer and the right is a null pointer constant.  
	if (IsPtrType(lty) && IsNullConstant(expr))
	{
		return 1;
	}

	if (IsPtrType(lty) && IsPtrType(rty))
	{
		#if 0
		Warning(&expr->coord, "assignment from incompatible pointer type");
		#endif
		return 1;
	}

	if ((IsPtrType(lty) && IsIntegType(rty) || IsPtrType(rty) && IsIntegType(lty))&&
		(lty->size == rty->size))
	{
		Warning(&expr->coord, "conversion between pointer and integer without a cast");
		return 1;
	}

	return 0;
}
/********************************************
	cast @expr's type to @ty
 ********************************************/
AstExpression Cast(Type ty, AstExpression expr)
{
	int scode = TypeCode(expr->ty);
	int dcode = TypeCode(ty);
	
	if (dcode == V)
	{
		return CastExpression(ty, expr);
	}
	if (scode < F4 && dcode < F4 && scode / 2 == dcode / 2)
	{
		// see examples/cast/sign.c
		int scateg = expr->ty->categ;
		int dcateg = ty->categ;
		if(scateg != dcateg && scateg >= INT && scateg <= ULONGLONG){			
			return CastExpression( ty,expr);
		}
		expr->ty = ty;
		return expr;
	}
	if (scode < I4)
	{
		expr = CastExpression(T(INT), expr);
		scode = I4;
	}	
	if (scode != dcode)
	{
		/***************************************************
			 int main(){
				 double c = 1.23;
				 char c2 = (char)c;
				 return 0;
			 }
			 --------------------------------
			 function main
				 c = 1.23;
				 t0 = (int)(double)c;
				 c2 = (char)(int)t0;
				 return 0;
				 ret

		 ****************************************************/
		if (dcode < I4)
		{
			expr = CastExpression(T(INT), expr);
		}
		expr = CastExpression(ty, expr);
	}
	return expr;
}
/*******************************************************

	void f(void){
	}
	int main(){
		while(f){	------------>  f is a FUNCTION, not scalar-type, 
				//  we have to adjust f to POINTER(FUNCTION) here
		}
	}
 *******************************************************/
AstExpression Adjust(AstExpression expr, int rvalue)
{
	//  see 	examples/qualfier/const.c
	int qual = 0;
	if (rvalue)
	{
		qual = expr->ty->qual;
		expr->ty = Unqual(expr->ty);
		expr->lvalue = 0;
		//PRINT_CUR_ASTNODE(expr);
	}

	if (expr->ty->categ == FUNCTION)
	{
		expr->ty = PointerTo(expr->ty);
		expr->isfunc = 1;
		//PRINT_I_AM_HERE();
	}
	else if (expr->ty->categ == ARRAY)
	{
		expr->ty = PointerTo(Qualify(qual,expr->ty->bty));
		/*************************************
			"abc" = "123";		// illegal
			(1)
			OP_STR	---->  OP_ID,	lvalue is 1
			here, we have to set lvalue to 0 explicitely
			(2)	for an id of array type, its lvalue is first set to 
				1 in CheckPrimaryExpression(AstExpression expr).				
		 *************************************/
		// PRINT_DEBUG_INFO(("lvalue = %x",expr->lvalue != 0));
		expr->lvalue = 0;
		expr->isarray = 1;
		//PRINT_CUR_ASTNODE(expr);
		
	}

	return expr;
}
// if epxr->ty is smaller than INT, promote to INT; otherwise, untouched.
AstExpression DoIntegerPromotion(AstExpression expr)
{
	return expr->ty->categ < INT ? Cast(T(INT), expr) : expr;
}

AstExpression CheckExpression(AstExpression expr)
{
	return (* ExprCheckers[expr->op])(expr);
}
//	This function check whether @expr is a int-const.
AstExpression CheckConstantExpression(AstExpression expr)
{
	expr = CheckExpression(expr);
	if (! (expr->op == OP_CONST && IsIntegType(expr->ty)))
	{
		return NULL;
	}
	return expr;
}

