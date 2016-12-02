#include "ucl.h"
#include "ast.h"
#include "expr.h"
#include "gen.h"

#define	GetBitFieldType(fld)	((fld->ty->categ %2) ? T(UINT):T(INT) )

/**
 * Translates a primary expression.
 */
static Symbol TranslatePrimaryExpression(AstExpression expr)
{
	if (expr->op == OP_CONST)
		return AddConstant(expr->ty, expr->val);

	/// if the expression is adjusted from an array or a function,
	/// returns the address of the symbol for this identifier
	if (expr->op == OP_STR || expr->isarray || expr->isfunc){
		//#if  1	// added
		//assert(expr->op != OP_STR);
		//#endif
		return AddressOf(expr->val.p);
	}

	return expr->val.p;
}

/**
 * Reading a bit field.
 * @param fld  bit field description 
 * @param p    the symbol which represents the whole integer containing the bit field
 */
/****************************************************************************
 struct Data{	 
		 int a;
		 int b:12;
		 int c; 
 };
 
 int main(){	 
	 struct Data dt;
	 int a; 	 
	 dt.b = 30;
	 a = dt.b; 
	 return 0;
 }

 function main
 	// dt.b = 30;		---------------------- WriteBitField
	 t1 = dt[4] & -4096;
	 t2 = t1 | 30;
	 dt[4] = t2;
	 // a = dt.b; 		----------------------  ReadBitField
	 t5 = dt[4] << 20;
	 t6 = t5 >> 20;	-----------			arithmetic right shift	(with sign)
	 a = t6;
	 return 0;
	 ret

 ****************************************************************************/
static Symbol ReadBitField(Field fld, Symbol p)
{
	int size  = 8 * fld->ty->size;
	int lbits = size - (fld->bits + fld->pos);
	int rbits = size - (fld->bits);

	/**
	 * Given the following code snippet:
	 * struct st
	 * {
	 *   int a;
	 *   int b : 3;
	 * } st;    
	 *          |bits(8)|    pos(16)   |        
	 * ---------------------------------
	 * |        |  b    |              |
	 * ---------------------------------
	 * In order to read b's value, at first, we must left shift b to the most significant bit
	 * then right shift b's to the least significant bit.
	 */
	// to be consistent with GCC/Clang, see examples/struct/bitfieldSign.c
	p = Simplify(T(INT),LSH, p, IntConstant(lbits));
	p = Simplify(GetBitFieldType(fld), RSH, p, IntConstant(rbits));	

	return p;
}

static Symbol WriteBitField(Field fld, Symbol dst, Symbol src)
{
	int fmask = (1 << fld->bits) - 1;
	int mask  = fmask << fld->pos;
	Symbol p;

	if(src->kind == SK_Constant && src->val.i[0] > fmask){
		Warning(dst->pcoord," overflow in implicit constant conversion");
	}

	if (src->kind == SK_Constant && src->val.i[0] == 0)
	{
		p = Simplify(T(INT), BAND, dst, IntConstant(~mask));
	}
	else if (src->kind == SK_Constant && (src->val.i[0] & fmask) == fmask)
	{
		p  = Simplify(T(INT), BOR, dst, IntConstant(mask));
	}
	else
	{
		if (src->kind == SK_Constant)
		{
			src = IntConstant((src->val.i[0] << fld->pos) & mask);
		}
		else
		{
			src = Simplify(T(INT), LSH, src, IntConstant(fld->pos));
			src = Simplify(T(INT), BAND, src, IntConstant(mask));
		}
		p = Simplify(T(INT), BAND, dst, IntConstant(~mask));
		p = Simplify(T(INT), BOR,  p, src);
	}

	if (dst->kind == SK_Temp && AsVar(dst)->def->op == DEREF)
	{
		/**********************************************
		 typedef struct{
			 int a;
			 int b:1;
		 }Data;
		 Data dt;
		 Data * ptr;
		 int main(){
			 ptr = &dt;
			 ptr->b = 1;
		 
			 return 0;
		 }
		----------------------------------------
		function main
			t0 :&dt;
			ptr = t0;
			t1 : ptr + 4;
			t2 :*t1;
			t3 : t2 | 1;
			*t1 = t3;
			return 0;
		 **********************************************/
		Symbol addr = AsVar(dst)->def->src1;

		GenerateIndirectMove(fld->ty, addr, p);
		dst = Deref(fld->ty, addr);
	}
	else
	{
		GenerateMove(fld->ty, dst, p);
	}
	return ReadBitField(fld, dst);
}



/**********************************************************
function Offset() only called in TranslateMemberAccess(AstExpression expr)
					and in TranslateArrayIndex(AstExpression expr)
	@ty		the type of an array element or a field of a struct
	@addr	basic address 
	@voff	variable offset,	index1 in arr[index1][3]
	@coff	constant offset,	3 in arr[index1][3]
 **********************************************************/
static Symbol Offset(Type ty, Symbol addr, Symbol voff, int coff)
{
	//PRINT_I_AM_HERE();
	if (voff != NULL)
	{	
		/****************************************************
			we get here after calling TranslateArrayIndex(AstExpression expr).
			For example:
				arr[index][2];
		 ****************************************************/
		//PRINT_DEBUG_INFO(("%s",addr->name));
		voff = Simplify(T(POINTER), ADD, voff, IntConstant(coff));
		addr = Simplify(T(POINTER), ADD, addr, voff);
		return Deref(ty, addr);
	}	
	//	TranslateMemberAccess/TranslateArrayIndex
	/*****************************************************************
		In fact, we don't need the following if-statement.
		But we can generate better assembly code with this if.
	 *****************************************************************/
	if (addr->kind == SK_Temp && AsVar(addr)->def->op == ADDR)
	{		
		/******************************************************
		we get here when	(1) arr[1][1][1]	(2)	dt.b
		for example:
		
		int arr[3][3][3];
		typedef struct{
			int a;
			int b;
		}Data;
		Data dt;
		int main(){
			arr[1][1][1] = 3;
			dt.b = 5;
			return 0;
		}
		see AddressOf(...),		arr	--->  addressof(arr)
						offset(): t0, arr
		 ******************************************************/
		//PRINT_DEBUG_INFO(("Offset(): %s, %s",addr->name,AsVar(addr)->def->src1->name));
		return CreateOffset(ty, AsVar(addr)->def->src1, coff,addr->pcoord);
	}
	/*****************************************************
		typedef struct{
			int a;
			int b;
		}Data;
		Data dt;
		Data * ptr = &dt;
		int main(){
			ptr-> a = 3;
			return 0;
		}

		or
		
		 int arr[4][4];
		 typedef int ARRAY[4][4];
		 ARRAY * ptr = &arr;
		 int main(){
			 (*ptr)[2][2] = 30;
			 return 0;
		}

		or 
		
		int arr[3][4];
		typedef int (*ArrPtr)[4];
		ArrPtr ptr = &arr[0];
		int main(){
			ptr[1][2] = 3;
			arr[1][2] = 3;
			return 0;
		}
	 *****************************************************/
	//PRINT_DEBUG_INFO(("%s",addr->name));
	return Deref(ty, Simplify(T(POINTER), ADD, addr, IntConstant(coff)));
}

/**
 * Generate intermedicate code to calculate a branch expression's value.
 * e.g. int a, b; a = a > b;
 * Introduces a new temporary t to holds the value of a > b.
 *     if a > b goto trueBB
 * falseBB:
 *     t = 0;
 *     goto nextBB;
 * trueBB:
 *     t = 1;
 * nextBB:
 *     ...
 */
 /***********************************************************
		int main(){
			int a = 3, b = 3;
			a = a > b;	----------  (a > b) is a branch expression
									convert logic value to arithmetic value (0/1)
			return 0;
		}

		function main
			a = 3;
			b = 3;
			if (a > b) goto BB0;
			a = 0;		-----------	falseBB
			goto BB1;
		BB0:				-----------	trueBB
			a = 1;
		BB1:				-----------  nextBB
			return 0;	
  ***********************************************************/
static Symbol TranslateBranchExpression(AstExpression expr)
{
	BBlock nextBB, trueBB, falseBB;
	Symbol t;

	t = CreateTemp(expr->ty);
	nextBB = CreateBBlock();
	trueBB = CreateBBlock();
	falseBB = CreateBBlock();

	TranslateBranch(expr, trueBB, falseBB);

	StartBBlock(falseBB);
	// a = 0;
	GenerateMove(expr->ty, t, IntConstant(0));
	// goto BB1
	GenerateJump(nextBB);

	StartBBlock(trueBB);
	GenerateMove(expr->ty, t, IntConstant(1));

	StartBBlock(nextBB);

	return t;
}

/**
 * Translates array index access expression.
 */
 /**********************************************************
	expr->op	  is  OP_INDEX
	//
	int main(){
		int arr[5][6];
		int x = 3;
		arr[x][4] = 10;
		arr[3][4] = 34;
		return 0;
	}
	//
	function main
		x = 3;
		// arr[x][4]
		t0 = x * 24;		-------	sizeof(int)*6 is 24
		t1 = &arr;
		t2 = t0 + 16;		-------  sizeof(int)*4 is 16
		t3 = t1 + t2;		-------  array addr + total offset
		*t3 = 10;
		// arr[3][4]
		arr[88] = 34;		-------	3*(sizeof(int)*6) + 4 * sizeof(int)  = 72 + 16 = 88
		return 0;
		ret

  **********************************************************/
static Symbol TranslateArrayIndex(AstExpression expr)
{
	AstExpression p;
	/*****************************************	
		int arr[3][5];
		int x = ...;
		arr[x][3];	----------	x is voff,  variable as offset 
							(x * sizeof(int)*5) is variable offset
							3 is coff,	 const as offset
							(3 * sizeof(int) ) is const offset
		we have construct the ast node for (3*sizeof(int) during semantics check.
		So the only thing here is to do add operation.
		To access an array element, we calculate its address as following:
			array_address	+ constant_offset + variable_offset
	 *****************************************/
	Symbol addr, dst, voff = NULL;
	int coff = 0;
	
	p = expr;
	/// 
	do
	{
		if (p->kids[1]->op == OP_CONST)
		{
			coff += p->kids[1]->val.i[0];
		}
		else if (voff == NULL)
		{
			voff = TranslateExpression(p->kids[1]);
		}
		else
		{
			voff = Simplify(voff->ty, ADD, voff, TranslateExpression(p->kids[1]));
		}
		p = p->kids[0];
		// see 	case OP_DEREF in CheckUnaryExpression()
	} while (p->op == OP_INDEX);

	
	addr = TranslateExpression(p);
	dst = Offset(expr->ty, addr, voff, coff);
	return expr->isarray ? AddressOf(dst) : dst;
}
/*******************************************************
 postfix-expression:
		 primary-expression
		 postfix-expression (  argument-expression-list<opt> ) 
		 
expe->op	is 	OP_CALL
 *******************************************************/
static Symbol TranslateFunctionCall(AstExpression expr)
{
	AstExpression arg;
	Symbol faddr, recv;
	ILArg ilarg;
	Vector args = CreateVector(4);
	/******************************************		
		Here, we want to use function name f as function call?
		Function name can be used as function address,
			see TranslatePrimaryExpression(AstExpression expr).
	 ******************************************/
	expr->kids[0]->isfunc = 0;
	faddr = TranslateExpression(expr->kids[0]);
	arg = expr->kids[1];
	while (arg)
	{
		ALLOC(ilarg);
		// After TranslateExpression(arg), sym->ty may be different from art->ty.
		// See AddressOf() and the following comments for detail.
		ilarg->sym = TranslateExpression(arg);
		ilarg->ty = arg->ty;
				
		INSERT_ITEM(args, ilarg);
		arg = (AstExpression)arg->next;
	}

	recv = NULL;
	if (expr->ty->categ != VOID)
	{
		recv = CreateTemp(expr->ty);		
	}
	GenerateFunctionCall(expr->ty, recv, faddr, args);

	return recv;
}
/**********************************************************
	 struct Data{		 
			 int a;
			 int b;
			 int c;	 
	 };
	 
	 int main(){		 
		 struct Data dt;
		 struct Data * ptr = &dt;
	 
		 ptr->b = 30;
		 dt.c = 50;
		 return 0;
	 }

	 //		see CreateOffset(...)
	 function main
		 t0 = &dt;
		 ptr = t0;
		 //	----------------	ptr->b	= 30
		 t1 = ptr + 4;
		 *t1 = 30;
		 //	dt.c = 50;
		 dt[8] = 50;
		 return 0;
		 ret


 **********************************************************/
static Symbol TranslateMemberAccess(AstExpression expr)
{
	AstExpression p;
	Field fld;
	Symbol addr, dst;
	int coff = 0;

	p = expr;
	if (p->op == OP_PTR_MEMBER)
	{
		/******************************************************
			ptr->b
				ptr is already an address.
			So we just use TranslateExpression(...) to get "ptr"				
		 *******************************************************/
		fld =  p->val.p;
		coff = fld->offset;
		addr = TranslateExpression(expr->kids[0]);
		//PRINT_DEBUG_INFO(("name = %s",addr->name));
	}
	else
	{
		/***************************************************
			dt.c
				dt is an struct, we have to get its address via
				AddressOf(...).
		 ***************************************************/
		while (p->op == OP_MEMBER)
		{
			fld = p->val.p;
			coff += fld->offset;
			p = p->kids[0];
		}
		addr = AddressOf(TranslateExpression(p));
	}
	//	No variable offset when accessing member of a struct.	
	//PRINT_DEBUG_INFO(("%s",addr->name));
	dst = Offset(expr->ty, addr, NULL, coff);	
	/******************************************
		store struct field object in symbol object for struct filed,
		to be used later  in 
		TranslateIncrement() and TranslateAssignmentExpression
	 ******************************************/
	fld = dst->val.p = expr->val.p;
	/****************************************
			typedef struct{
				int a;
				int b:1;
			}Data;
			Data dt;
			int main(){
				int a ;
				dt.b = 1;		// lvalue is 1
				a = dt.b;		// lvalue is 0
				return 0;
			}	
			function main
				t1 : dt[4] | 1;		// writing
				dt[4] = t1;
				
				t4 : dt[4] << 31;		// reading
				t5 : t4 >> 31;
				a = t5;
				return 0;
	If we want to read the value of bitfiled, read it.
	The result after reading is a temporary, as t5 above.
	 ****************************************/
	// PRINT_DEBUG_INFO(("lvalue = %d",expr->lvalue));
	if (fld->bits != 0 && expr->lvalue == 0){
		//PRINT_CUR_ASTNODE(expr);
		return ReadBitField(fld, dst);
	}
	
	return expr->isarray ? AddressOf(dst) : dst;
}
/****************************************************************
	see semantics for OP_POSTINC in function TransformIncrement(AstExpression expr)
		the ast node for OP_POSTINC is changed during semantics checking.
 ****************************************************************/
static Symbol TranslateIncrement(AstExpression expr)
{
	AstExpression casgn;
	Symbol p;
	Field fld = NULL;

	casgn = expr->kids[0];
	/*********************************************************
	The reason why we mark casgn->kids[0] as OP_ID is same as
		that in TranslateAssignmentExpression().
	 **********************************************************/
	p = TranslateExpression(casgn->kids[0]);	
	//PRINT_DEBUG_INFO(("%s",p->name));
	casgn->kids[0]->op = OP_ID;
	casgn->kids[0]->val.p = p;
	fld = p->val.p;

	if (expr->op == OP_POSTINC || expr->op == OP_POSTDEC)
	{		
		Symbol ret;

		ret = p;
		/********************************************************
		 int num[9];
		 int * ptr = num;
		 int main(){
			 (*ptr)++;
			 return 0;
		 }
		---------------------------------
		function main
			t0 :*ptr;
			t1 : t0 + 1;
			*ptr = t1;
			return 0;
		 ********************************************************/
		//PRINT_DEBUG_INFO(("symbol: %s", GetSymbolKind(p->kind)));
		if (casgn->kids[0]->bitfld)
		{
			ret = ReadBitField(fld, p);
		}
		else if (p->kind != SK_Temp)
		{			
			ret = CreateTemp(expr->ty);
			GenerateMove(expr->ty, ret, p);
		}
		TranslateExpression(casgn);
		return ret;
	}

	return TranslateExpression(casgn);
}

static Symbol TranslatePostfixExpression(AstExpression expr)
{
	switch (expr->op)
	{
	case OP_INDEX:
		return TranslateArrayIndex(expr);

	case OP_CALL:
		return TranslateFunctionCall(expr);

	case OP_MEMBER:
	case OP_PTR_MEMBER:
		return TranslateMemberAccess(expr);

	case OP_POSTINC:
	case OP_POSTDEC:
		return TranslateIncrement(expr);

	default:
		assert(0);
		return NULL;
	}
}
/************************************************************
 OPCODE(EXTI1,	 "(int)(char)", 		 Cast)
 OPCODE(EXTU1,	 "(int)(unsigned char)", Cast)
 OPCODE(EXTI2,	 "(int)(short)",		 Cast)
 OPCODE(EXTU2,	 "(int)(unsigned short)",Cast)
 OPCODE(TRUI1,	 "(char)(int)", 		 Cast)
 OPCODE(TRUI2,	 "(short)(int)",		 Cast)
 OPCODE(CVTI4F4, "(float)(int)",		 Cast)
 OPCODE(CVTI4F8, "(double)(int)",		 Cast)
 OPCODE(CVTU4F4, "(float)(unsigned)",	 Cast)
 OPCODE(CVTU4F8, "(double)(unsigned)",	 Cast)
 OPCODE(CVTF4,	 "(double)(float)", 	 Cast)
 OPCODE(CVTF4I4, "(int)(float)",		 Cast)
 OPCODE(CVTF4U4, "(unsigned)(float)",	 Cast)
 OPCODE(CVTF8,	 "(float)(double)", 	 Cast)
 OPCODE(CVTF8I4, "(int)(double)",		 Cast)
 OPCODE(CVTF8U4, "(unsigned)(double)",	 Cast)

 ************************************************************/
static Symbol TranslateCast(Type ty, Type sty, Symbol src)
{
	Symbol dst;
	int scode, dcode, opcode;

	dcode = TypeCode(ty);
	scode = TypeCode(sty);

	if(scode < I4){
		assert(dcode == I4);
	}
	if(scode == dcode){
		// for examples:	unsigned int ---> unsigned long
		//PRINT_DEBUG_INFO(("%s,%d:%s--->%s",src->pcoord->filename,src->pcoord->ppline,
		//					TypeToString(sty), TypeToString(ty)));
		return src;
	}

	if (dcode == V)
		return NULL;
	//
	switch (scode)
	{
	/******************************************
		I1,I2,U1,U2 are EXTended to int implicitely.
		see Cast()
		
		float f32 = 1.23f;
		char ch = 'a';
		f32 = (float)ch;

			f32 = 1.23;
			ch = 97;
			t0 = (int)(char)ch;	-----------(char) --> (int)		implicitely
			f32 = (float)(int)t0;
	 ******************************************/
	case I1:
		opcode = EXTI1; break;

	case I2:
		opcode = EXTI2; break;

	case U1:
		opcode = EXTU1; break;

	case U2:
		opcode = EXTU2; break;

	case I4:
		/**************************************
		I4 are TRUncated to I1,I2
			  ConVerTed to float, double
		 **************************************/
		if (dcode <= U1)
			opcode = TRUI1;
		else if (dcode <= U2)
			opcode = TRUI2;
		else if (dcode == F4)
			opcode = CVTI4F4;
		else if (dcode == F8)
			opcode = CVTI4F8;
		else{
			/*************************************************
			short a = -1;
			int main(int argc,char * argv[]){
				printf("%x \n",((unsigned int)a) >> 1);
				return 0;
			}
			//	short	-->		int	-->		unsigned int
			 **************************************************/	
			// see examples/cast/sign.c
			Symbol temp = CreateTemp(ty);
			GenerateMove(ty,temp,src);
			return temp;
		}
		break;

	case U4:
		if (dcode == F4)
			opcode = CVTU4F4;
		else if (dcode == F8)
			opcode = CVTU4F8;
		else{
			Symbol temp = CreateTemp(ty);
			GenerateMove(ty,temp,src);
			return temp;	
		}
		break;

	case F4:
		if (dcode == I4)
			opcode = CVTF4I4;
		else if (dcode == U4)
			opcode = CVTF4U4;
		else
			opcode = CVTF4;
		break;

	case F8:
		if (dcode == I4)
			opcode = CVTF8I4;
		else if (dcode == U4)
			opcode = CVTF8U4;
		else
			opcode = CVTF8;
		break;

	default:
		assert(0);
		return NULL;
	}

	dst = CreateTemp(ty);	
	GenerateAssign(ty, dst, opcode, src, NULL);
	return dst;
	
}
/************************************************************************
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
 ************************************************************************/
static Symbol TranslateUnaryExpression(AstExpression expr)
{
	Symbol src;

	if (expr->op == OP_NOT)
	{
		return TranslateBranchExpression(expr);
	}

	if (expr->op == OP_PREINC || expr->op == OP_PREDEC)
	{
		return TranslateIncrement(expr);
	}

	src = TranslateExpression(expr->kids[0]);
	switch (expr->op)
	{
	case OP_CAST:	//	(int) a
		//PRINT_DEBUG_INFO(("%s --> %s",TypeToString(expr->kids[0]->ty),TypeToString(expr->ty)));
		return TranslateCast(expr->ty, expr->kids[0]->ty, src);

	case OP_ADDRESS:	// &a		
		return AddressOf(src);

	case OP_DEREF:		// *a
		return Deref(expr->ty, src);

	case OP_NEG:
	case OP_COMP:		//	+/-
		return Simplify(expr->ty, OPMap[expr->op], src, NULL);

	default:
		assert(0);
		return NULL;
	}
}

static Symbol TranslateBinaryExpression(AstExpression expr)
{
	Symbol src1, src2;

	if (expr->op == OP_OR || expr->op == OP_AND || (expr->op >= OP_EQUAL && expr->op <= OP_LESS_EQ))
	{
		return TranslateBranchExpression(expr);
	}
	src1 = TranslateExpression(expr->kids[0]);
	src2 = TranslateExpression(expr->kids[1]);

	return Simplify(expr->ty, OPMap[expr->op], src1, src2);
}
/************************************************************************
 Syntax
 
		   conditional-expression:
				   logical-OR-expression
				   logical-OR-expression ?	expression :  conditional-expression

 *************************************************************************/
static Symbol TranslateConditionalExpression(AstExpression expr)
{
	Symbol t, t1, t2;
	BBlock trueBB, falseBB, nextBB;

	t = NULL;
	/**************************************************
	 void f(){
		 printf("%s\n",__FUNCTION__);
	 }
	 void g(){
		 printf("%s\n",__FUNCTION__);
	 }
	 int main(){
		 int a = 3;
		 (a>3)?f():g();
	 
		 return 0;
	 }
	 ***************************************************/
	if (expr->ty->categ != VOID)
	{
		t = CreateTemp(expr->ty);
	}
	trueBB = CreateBBlock();
	falseBB = CreateBBlock();
	nextBB = CreateBBlock();
	
	// to be consistent with if(){}else{}
	TranslateBranch(Not(expr->kids[0]), falseBB,trueBB);

	StartBBlock(trueBB);
	t1 = TranslateExpression(expr->kids[1]->kids[0]);
	if (t1 != NULL)
		GenerateMove(expr->ty, t, t1);
	GenerateJump(nextBB);

	StartBBlock(falseBB);
	t2 = TranslateExpression(expr->kids[1]->kids[1]);
	if (t2 != NULL)
		GenerateMove(expr->ty, t, t2);

	StartBBlock(nextBB);
	return t;
}

static Symbol TranslateAssignmentExpression(AstExpression expr)
{
	Symbol dst, src;
	Field fld = NULL;

	dst = TranslateExpression(expr->kids[0]);
	fld = dst->val.p;
	/****************************************
	 OPINFO(OP_COMMA,		  1,	",",	  Comma,		  NOP)
	 OPINFO(OP_ASSIGN,		  2,	"=",	  Assignment,	  NOP)
	 OPINFO(OP_BITOR_ASSIGN,  2,	"|=",	  Assignment,	  NOP)
	 OPINFO(OP_BITXOR_ASSIGN, 2,	"^=",	  Assignment,	  NOP)
	 OPINFO(OP_BITAND_ASSIGN, 2,	"&=",	  Assignment,	  NOP)
	 OPINFO(OP_LSHIFT_ASSIGN, 2,	"<<=",	  Assignment,	  NOP)
	 OPINFO(OP_RSHIFT_ASSIGN, 2,	">>=",	  Assignment,	  NOP)
	 OPINFO(OP_ADD_ASSIGN,	  2,	"+=",	  Assignment,	  NOP)
	 OPINFO(OP_SUB_ASSIGN,	  2,	"-=",	  Assignment,	  NOP)
	 OPINFO(OP_MUL_ASSIGN,	  2,	"*=",	  Assignment,	  NOP)
	 OPINFO(OP_DIV_ASSIGN,	  2,	"/=",	  Assignment,	  NOP)
	 OPINFO(OP_MOD_ASSIGN,	  2,	"%=",	  Assignment,	  NOP)
	 OPINFO(OP_QUESTION,	  3,	"?",	  Conditional,	  NOP)

	 ****************************************/
	if (expr->op != OP_ASSIGN)
	{
		/**********************************************************
			#include <stdio.h>
			 int arr[3][4][5];
			 int * f(void){
				 static int number;
				 printf("int * f(void)\n");
				 return &number;
			 }
			 int main(){ 
				 arr[1][2][3] += 3;  
				 *f()	 += 3;
				 return 0;
			 }

		dst is the result of arr[1][2][3],which we get from TranslateExpression(expr->kids[1]) .
		But when we call TranslateExpression(expr->kids[1]) later,
		arr[1][2][3] will be evaluated again.
		So here, we avoid evaluating arr[1][2][3] again by marking 
		expr->kids[0] as an OP_ID node, which will be processed only by
		TranslatePrimaryExpression().
		If we comment the following two lines, we will get two lines of "int * f(void)",
		which is not correct semantics.
		 **********************************************************/
		//PRINT_DEBUG_INFO(("op = %s",OPNames[expr->kids[0]->op]));
		expr->kids[0]->op = OP_ID;
		expr->kids[0]->val.p = expr->kids[0]->bitfld ? ReadBitField(fld, dst) : dst;
	}
	src = TranslateExpression(expr->kids[1]);

	if (expr->kids[0]->bitfld)
	{
		return WriteBitField(fld, dst, src);
	}
	else if (dst->kind == SK_Temp && AsVar(dst)->def->op == DEREF)
	{
		/*******************************************
			see	the examples in TranslateIncrement(AstExpression expr)
		 *******************************************/
		Symbol addr = AsVar(dst)->def->src1;
		//PRINT_DEBUG_INFO(("name = %s",dst->name));
		GenerateIndirectMove(expr->ty, addr, src);
		dst = Deref(expr->ty, addr);
	}
	else
	{
		GenerateMove(expr->ty, dst, src);
	}
	return dst;
}

static Symbol TranslateCommaExpression(AstExpression expr)
{
	TranslateExpression(expr->kids[0]);
	return TranslateExpression(expr->kids[1]);
}

static Symbol TranslateErrorExpression(AstExpression expr)
{
	assert(0);
	return NULL;
}
/***********************************************
OPINFO(OP_DIV,           13,   "/",      Binary,         DIV)
OPINFO(OP_MOD,           13,   "%",      Binary,         MOD)
OPINFO(OP_CAST,          14,   "cast",   Unary,          NOP)
OPINFO(OP_PREINC,        14,   "++",     Unary,          NOP)
OPINFO(OP_PREDEC,        14,   "--",     Unary,          NOP)
OPINFO(OP_ADDRESS,       14,   "&",      Unary,          ADDR)
OPINFO(OP_DEREF,         14,   "*",      Unary,          DEREF)
 ***********************************************/
static Symbol (* ExprTrans[])(AstExpression) = 
{
#define OPINFO(op, prec, name, func, opcode) Translate##func##Expression,
#include "opinfo.h"
#undef OPINFO
};
/***********************************************************************
	This function is only used in 
		TranslateBranch(...);
	During exprchk.c,
		we can't replace every (!!a)	with (a)
		because	
			(!!a)	+3 
			is different from 
			a+3
 ***********************************************************************/
AstExpression Not(AstExpression expr)
{
	static int rops[] = { OP_UNEQUAL, OP_EQUAL, OP_LESS_EQ, OP_GREAT_EQ, OP_LESS, OP_GREAT };
	AstExpression t;

	switch (expr->op)
	{
	case OP_OR:
		expr->op = OP_AND;
		expr->kids[0] = Not(expr->kids[0]);
		expr->kids[1] = Not(expr->kids[1]);
		return expr;

	case OP_AND:
		expr->op = OP_OR;
		expr->kids[0] = Not(expr->kids[0]);
		expr->kids[1] = Not(expr->kids[1]);
		return expr;

	case OP_EQUAL:
	case OP_UNEQUAL:
	case OP_GREAT:
	case OP_LESS:
	case OP_GREAT_EQ:
	case OP_LESS_EQ:
		expr->op = rops[expr->op - OP_EQUAL];
		return expr;

	case OP_NOT:
		return expr->kids[0];

	default:
		CREATE_AST_NODE(t, Expression);
		t->coord = expr->coord;
		t->ty = T(INT);
		t->op = OP_NOT;
		t->kids[0] = expr;
		return FoldConstant(t);
	}
}
/*******************************************************************
	if(expr)	goto trueBB.
	@expr	the expression for test
	@trueBB
	@falseBB	
			We need falseBB only when we are translating short-cut	&&  ||
			the @expr needs to jump to falseBB.
 *******************************************************************/
void TranslateBranch(AstExpression expr, BBlock trueBB, BBlock falseBB)
{
	BBlock rtestBB;
	Symbol src1, src2;
	Type ty;

	switch (expr->op)
	{
	case OP_AND:
		/***************************************************
			a && b
				rtestBB		right test basic block, here ,'b' is right test.
		 ****************************************************/
		rtestBB = CreateBBlock();
		TranslateBranch(Not(expr->kids[0]), falseBB, rtestBB);
		/*************************************************************
		we can't use :
			TranslateBranch(expr->kids[0], rtestBB,falseBB);
		see the result of the following example:
			int a,b;
			int main(){
				a&&b;
				return 0;
			}
			------------------------------------
			function main
				if (a) goto BB1;	// No matter a is true or not, we always goto BB1.
			BB1: 
				if (b) goto BB3;
			BB2: 
				t0 = 0;
				goto BB4;
			BB3: 
				t0 = 1;
			BB4: 
				return 0;
		The reason is that:
			we only generate one conditional jump for >=, == ,....,  !, OP_CONST, ..
			the default action is if the condition is true, we jump to truePart,
			if it fails, we fall to the next statement which should be the beginning
			of the falsePart.	
			TranslateBranch(a>0, trueBB, falseBB);
			a > 0:
				if (a > 0) goto BB2;
				BB1: 	----------- Here starts the FalseBB
					t0 = 0;
					goto BB3;
				BB2: 
					t0 = 1;
				BB3: 
					return 0;
		So the basic pattern to use TranslateBranch(expr,truePart,falsePart) is:
			(1)TranslateBranch(expr,trueBB,falseBB);
			(2)StartBBlock(falseBB);
		For example:
			TranslateBranch(a&&b,trueBB,falseBB);
			
			StartBBlock(falsePart);
		are converted to the following calls:
			TranslateBranch(!a,falseBB,rtestBB);
			StartBBlock(rtestBB);
			TranslateBranch(b,trueBB,falseBB);

			StartBBlock(falsePart);
		
		 *************************************************************/
		StartBBlock(rtestBB);
		TranslateBranch(expr->kids[1], trueBB, falseBB);
		/************************************************************
			when we call TranslateBranch(AstExpression expr, BBlock trueBB, BBlock falseBB),
			according to the pattern described before,
			we know it is called as:
				TranslateBranch(expr, trueBB, falseBB);
				StartBlock(falseBB);
			So we don't have to add another "StartBlock(falseBB)" here.
		 *************************************************************/
		break;

	case OP_OR:
		rtestBB = CreateBBlock();
		TranslateBranch(expr->kids[0], trueBB, rtestBB);
		StartBBlock(rtestBB);
		TranslateBranch(expr->kids[1], trueBB, falseBB);
		break;

	case OP_EQUAL:
	case OP_UNEQUAL:
	case OP_GREAT:
	case OP_LESS:
	case OP_GREAT_EQ:
	case OP_LESS_EQ:
		src1 = TranslateExpression(expr->kids[0]);
		src2 = TranslateExpression(expr->kids[1]);
		GenerateBranch(expr->kids[0]->ty, trueBB, OPMap[expr->op], src1, src2);
		break;

	case OP_NOT:		
		// for better  if(!!!!!a)	, here we treat it as bool expression, also see TranslateUnaryExpression()
		{
			int count = 1;
			AstExpression parent = expr;
			AstExpression child = expr->kids[0];
			while(child->op == OP_NOT){
				parent = child;
				child = child->kids[0];
				count++;
			}
			src1 = TranslateExpression(parent->kids[0]);
			ty = parent->kids[0]->ty;
			if (ty->categ < INT)
			{
				src1 = TranslateCast(T(INT), ty, src1);
				ty = T(INT);
			}
			if(count % 2 == 1){
				GenerateBranch(ty, trueBB, JZ, src1, NULL);
			}else{
				GenerateBranch(ty, trueBB, JNZ, src1, NULL);
			}
		}
		break;

	case OP_CONST:
		if (! (expr->val.i[0] == 0 && expr->val.i[1] == 0))
		{			
			GenerateJump(trueBB);
		}
		break;

	default:
		
		src1 = TranslateExpression(expr);		
		if (src1->kind  == SK_Constant)
		{
			/**********************************************************
			It seems that statements like "( a || (b, 9) );;"  get here.
				int a ,b,c;
				int main(){
					( a || (b, 9) );
					return 0;
				}
			**********************************************************/
			if (! (src1->val.i[0] == 0 && src1->val.i[1] == 0))
			{
				GenerateJump(trueBB);
			}
		}
		else
		{
			ty = expr->ty;
			if (ty->categ < INT)
			{
				src1 = TranslateCast(T(INT), ty, src1);
				ty = T(INT);
			}
			GenerateBranch(ty, trueBB, JNZ, src1, NULL);
		}
		break;
	}
}

Symbol TranslateExpression(AstExpression expr)
{
	return (* ExprTrans[expr->op])(expr);
}
