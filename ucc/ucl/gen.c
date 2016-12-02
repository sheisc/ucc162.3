#include "ucl.h"
#include "gen.h"

#include "ast.h"
#include "expr.h"
#include "input.h"

#define IsCommute(op) 0

BBlock CurrentBB;
/**********************************************************
	OPINFO(OP_COMMA,		 1,    ",", 	 Comma, 		 NOP)
	OPINFO(OP_ASSIGN,		 2,    "=", 	 Assignment,	 NOP)
	OPINFO(OP_BITOR_ASSIGN,  2,    "|=",	 Assignment,	 NOP)
	OPINFO(OP_BITXOR_ASSIGN, 2,    "^=",	 Assignment,	 NOP)
	.......
	OPINFO(OP_QUESTION, 	 3,    "?", 	 Conditional,	 NOP)
	OPINFO(OP_COLON,		 3,    ":", 	 Error, 		 NOP)
	.......
	OPINFO(OP_EQUAL,		 9,    "==",	 Binary,		 JE)
	OPINFO(OP_UNEQUAL,		 9,    "!=",	 Binary,		 JNE)
	OPINFO(OP_GREAT,		 10,   ">", 	 Binary,		 JG)
	OPINFO(OP_LESS, 		 10,   "<", 	 Binary,		 JL)

	prec:		precedence
 ***********************************************************/
int OPMap[] = 
{
#define OPINFO(tok, prec, name, func, opcode) opcode,
#include "opinfo.h"
#undef  OPINFO
};
/****************************************************
	we are sure that :
			p->kind == SK_Variable
	p is for local/global/static variable, not for temporary variable.
 ****************************************************/
static void TrackValueChange(Symbol p)
{
	ValueUse use = AsVar(p)->uses;

	assert(p->kind == SK_Variable);

	while (use)
	{
		/***********************************************
			mark the definitions useing @p as operands invalid.
			Unless it contains the address of a variable.
			The content of a variable changes while its address not.
			So the temporary is still valid in this case.
				t1 = &var;	------ var changes, but &var not.
				
		 ***********************************************/	
		if (use->def->op != ADDR)
			use->def->dst = NULL;

		use = use->next;
	}
}
/***************************************************
	record that @p is used in defintion @def.
	we are sure that:
		p->kind == SK_Variable
	We never track the temporary-variable, only track local/global/static
	variable used in defintion of temporary.
		t3 = a + b
	If a is changed , t3 is made invalid via 
	But no track in the following situtation.
		c = a + b		
 ***************************************************/
static void TrackValueUse(Symbol p, ValueDef def)
{
	ValueUse use;

	assert(p->kind == SK_Variable);

	ALLOC(use);
	use->def = def;
	use->next = AsVar(p)->uses;
	AsVar(p)->uses = use;
}

/***********************************************************
	(1)see UCC document 9.2
		UCC only allocates register for temporary.
	(2)	t1 = a + 3, is a definition of t1
	(3)	a + b + c
		we have to allocate t2 for (a+b),
		it is also considered as a temporary definition of t2.
	(4)		
		if (b) goto BB1;
			t0 = 5;	-----		t0	MOV		5
			goto BB2;
		BB1:
			t0 = 3;	-----		t0	MOV		3
		BB2:
			a = t0;
 ***********************************************************/
void DefineTemp(Symbol t, int op, Symbol src1, Symbol src2)
{
	ValueDef def;

	ALLOC(def);
	def->dst = t;
	def->op = op;
	def->src1 = src1;	//	If op is MOV/CALL,  src1 is an  (IRInst) .see PeepHole(BBlock bb) for MOV	
	def->src2 = src2;
	def->ownBB = CurrentBB;
	

	assert(t->kind == SK_Temp);	
	
	if (op == MOV || op == CALL)
	{		
		def->link = AsVar(t)->def;
		AsVar(t)->def = def;
		return;
	}

	if (src1->kind == SK_Variable)
	{
		/***********************************
			t = src1 op src2
			the definition of 't' here use @src1.
			In other words, @src1 is used in this definition.			
		 ***********************************/
		TrackValueUse(src1, def);
	}
	if (src2 && src2->kind == SK_Variable)
	{
		TrackValueUse(src2, def);
	}
	AsVar(t)->def = def;
}
/*****************************************************
	(1)
		The returned bblock of CreateBBlock(...) is not named yet.
	(2)	The naming operation is done later: 
		bb->sym is created later via CreateLabel() called 
	in TranslateFunction(AstFunction func).
	(3)	Some basic blocks will be merged later in Optimize(),
	no name is needed for them.
 ******************************************************/
BBlock CreateBBlock(void)
{
	BBlock bb;

	CALLOC(bb);

	bb->insth.opcode = NOP;
	bb->insth.prev = bb->insth.next = &bb->insth;
	return bb;
}
/***********************************************
	(1)
	When generating UIL or assembly code, we just have 
	to generate code  from top to down 
	in a file. So a block-list is efficient for such task.
	(2)
	This function add @bb into basic block list.
 ***********************************************/
void StartBBlock(BBlock bb)
{
	IRInst lasti;

	CurrentBB->next = bb;
	bb->prev = CurrentBB;
	lasti = CurrentBB->insth.prev;
	/*************************************************
	BB1:
		s1;
		......
		sn;
	BB2:
		t1;
		..
		tm
	If the above sn is not Jmp/IJMP,
		we are sure that  the control could flow directly from sn to t1;
		that is , a cfg-edge exists from BB1 to BB2 here,
		Even sn be a conditional jump.
	while sn is a Jmp/Ijmp, its target may be BB2, 
		we draw CfgEdge for them in GenerateJump(BBlock dstBB) and 
		GenerateIndirectJump(BBlock *dstBBs, int len, Symbol index)
		respectively.

	Warning:
		It seems that DrawCFGEdge don't check whether there is already 
		an same edge in the CFG.
	 *************************************************/
	if (lasti->opcode != JMP && lasti->opcode != IJMP)
	{
		DrawCFGEdge(CurrentBB, bb);
	}
	CurrentBB = bb;
}

void AppendInst(IRInst inst)
{
	assert(CurrentBB != NULL);

	CurrentBB->insth.prev->next = inst;
	inst->prev = CurrentBB->insth.prev;
	inst->next = &CurrentBB->insth;
	CurrentBB->insth.prev = inst;

	CurrentBB->ninst++;
}
/**********************************************************
	d = a + b +c
	Inner structure in UCC
	t0:		a + b	----------		This is not MOV
	t1:		t0 + c	----------		This is not MOV
	d	MOV	  t1
	Printable uil
	t0  = a +b
	t1 = t0 +c
	d = t1

	The rare case we have to generate something like
		t3	MOV	  d
	is 
		a > 1 ? 3 : 4
 ***********************************************************/
void GenerateMove(Type ty, Symbol dst, Symbol src)
{
	IRInst inst;

	ALLOC(inst);
	dst->ref++;
	src->ref++;
	inst->ty = ty;
	inst->opcode  = MOV;
	inst->opds[0] = dst;
	inst->opds[1] = src;
	inst->opds[2] = NULL;
	AppendInst(inst);
	/*************************************************
		If a named variable is the destination of MOV,
		all the calculated subexpression using this variable is invalid.
	 ************************************************/
	if (dst->kind == SK_Variable)
	{
		TrackValueChange(dst);
	}
	else if (dst->kind == SK_Temp)
	{
		DefineTemp(dst, MOV, (Symbol)inst, NULL);
	}
}
/********************************************************
see static void DAssemUIL(IRInst inst)
			and static void EmitIndirectMove(IRInst inst).

	[addr] = 30;	---------> because temporary has no address,
		so [addr] = 30  isn't the definition of a temporary.
		That is , no need to call DefineTemp(...) here.
 ********************************************************/
void GenerateIndirectMove(Type ty, Symbol dst, Symbol src)
{
	IRInst inst;

	ALLOC(inst);
	dst->ref++;
	src->ref++;
	inst->ty = ty;
	inst->opcode  = IMOV;
	inst->opds[0] = dst;
	inst->opds[1] = src;
	inst->opds[2] = NULL;
	AppendInst(inst);
}
/**********************************************************************
		int a, b;
		b = 4;
		a = 3 + b;		----------	In UIL,  3 + b is an assignment to t0
						----------	then mov t0 to a
	
		
	b = 4;
	t0 = b + 3;
	a = t0;

	dst:
		src1	+ src2
		src1 | src2
		........
 **********************************************************************/
void GenerateAssign(Type ty, Symbol dst, int opcode, Symbol src1, Symbol src2)
{
	IRInst inst;

	assert(dst->kind == SK_Temp);

	ALLOC(inst);
	dst->ref++;
	src1->ref++;
	if (src2) src2->ref++;
	inst->ty = ty;
	inst->opcode = opcode;
	inst->opds[0] = dst;
	inst->opds[1] = src1;
	inst->opds[2] = src2;
	AppendInst(inst);

	DefineTemp(dst, opcode, src1, src2);
}
/*************************************************************************
	GenerateBranch(choice->ty, lhalfBB, JL, choice, IntConstant(bucketArray[mid]->minVal));
	output:
		if (a < 1) goto BB5;
	@ty			the type of 'a'
	@dstBB		BB5
	@opcode		JL
	@src1		'a'
	@src2		1
 *************************************************************************/
void GenerateBranch(Type ty, BBlock dstBB, int opcode, Symbol src1, Symbol src2)
{
	IRInst inst;

	ALLOC(inst);
	dstBB->ref++;
	src1->ref++;
	if (src2) src2->ref++;
	DrawCFGEdge(CurrentBB, dstBB);
	inst->ty = ty;
	inst->opcode  = opcode;
	inst->opds[0] = (Symbol)dstBB;
	inst->opds[1] = src1;
	inst->opds[2] = src2;
	AppendInst(inst);
}

void GenerateJump(BBlock dstBB)
{
	IRInst inst;

	ALLOC(inst);
	dstBB->ref++;
	DrawCFGEdge(CurrentBB, dstBB);
	inst->ty = T(VOID);
	inst->opcode = JMP;
	inst->opds[0] = (Symbol)dstBB;
	inst->opds[1] = inst->opds[2] = NULL;
	AppendInst(inst);
}
/**************************************************************
	 int main(){
		 int a = 3;
		 switch(a){
		 case 1:
			 a += 1;
			 break;
		 case 2:
			 a += 2;
			 break;
		 case 3:
			 a += 3;
			 break;
		 }
		 return 0;
	 }

	 

	 a = 3;
	 if (a < 1) goto BB4;
	 if (a > 3) goto BB4;
	 t0 = a - 1;
	 goto (BB1,BB2,BB3,)[t0];	---------IndirectJump
 BB1:
	 t1 = a + 1;
	 a = t1;
	 goto BB4;
 BB2:
	 t2 = a + 2;
	 a = t2;
	 goto BB4;
 BB3:
	 t3 = a + 3;
	 a = t3;
	 goto BB4;
 BB4:
	 return 0;
	 goto BB5;

 **************************************************************/
void GenerateIndirectJump(BBlock *dstBBs, int len, Symbol index)
{
	IRInst inst;
	int i;
	
	ALLOC(inst);
	index->ref++;
	for (i = 0; i < len; ++i)
	{
		dstBBs[i]->ref++;
		DrawCFGEdge(CurrentBB, dstBBs[i]);
	}
	inst->ty = T(VOID);
	inst->opcode = IJMP;
	inst->opds[0] = (Symbol)dstBBs;
	inst->opds[1] = index;
	inst->opds[2] = NULL;
	AppendInst(inst);
}
/***********************************************
 int f(){
	 int a = 3;
	 return a+5;
 }

 function f
	 a = 3;
	 t0 = a + 5;
	 return t0;

 ***********************************************/
void GenerateReturn(Type ty, Symbol src)
{
	IRInst inst;

	ALLOC(inst);
	src->ref++;
	inst->ty = ty;
	inst->opcode = RET;
	inst->opds[0] = src;
	inst->opds[1] = inst->opds[2] = NULL;
	AppendInst(inst);
}
/********************************************************************
	int f(int a, int b){
		
		return a+b;
	}
	
	int main(){
		int a = 3, b;
		b = f(a,5);
		return 0;
	}

	t0 = f(a, 5);
	b = t0;	
 ********************************************************************/
void GenerateFunctionCall(Type ty, Symbol recv, Symbol faddr, Vector args)
{
	ILArg p;
	IRInst inst;

	ALLOC(inst);
	if (recv) recv->ref++;
	faddr->ref++;
	FOR_EACH_ITEM(ILArg, p, args)
		p->sym->ref++;
	ENDFOR
	inst->ty = ty;
	inst->opcode = CALL;
	inst->opds[0] = recv;
	inst->opds[1] = faddr;
	inst->opds[2] = (Symbol)args;
	AppendInst(inst);

	if (recv != NULL)
		DefineTemp(recv, CALL, (Symbol)inst, NULL);
}

void GenerateClear(Symbol dst, int size)
{
	IRInst inst;

	ALLOC(inst);
	dst->ref++;
	inst->ty = T(UCHAR);
	inst->opcode = CLR;
	inst->opds[0] = dst;
	inst->opds[1] = IntConstant(size);
	inst->opds[1]->ref++;
	inst->opds[2] = NULL;
	AppendInst(inst);
}

Symbol AddressOf(Symbol p)
{
	if (p->kind == SK_Temp && AsVar(p)->def->op == DEREF)
	{		
		return AsVar(p)->def->src1;
	}

	assert(p->kind != SK_Temp);

	p->addressed = 1;
	if (p->kind == SK_Variable)
	{
		TrackValueChange(p);
	}
	return TryAddValue(T(POINTER), ADDR, p, NULL); 
}


Symbol Deref(Type ty, Symbol addr)
{
	Symbol tmp;
	if (addr->kind == SK_Temp && AsVar(addr)->def->op == ADDR)
	{
		/******************************
			t1 = &var;	---------  t1 is a temporary holding var's address.
			*t1 is var.
		 ******************************/
		//#if 1	// added, we never get here? We already delete redundant &var in Offset()?
		//assert(0);
		//#endif
		return AsVar(addr)->def->src1;
	}
	tmp = CreateTemp(ty);
	GenerateAssign(ty, tmp, DEREF, addr, NULL);
	return tmp;
}
/***********************************************************
	If we try to do : 	a + b
	we first find in the hashtable whether it is a common subexpression
	calculated before.
 (1)
	int a = 1, b = 2 , c = 3, d;
	d = a + b + c;	
	d = a + b
	c = a + b + c;


	t0 = a + b;
	t1 = t0 + c;
	d = t1;
	d = t0;		------------	a + b is still valid in t0
	c = t1;		------------	a + b + c is still valid in t1

(2)
	 d = a + b + c;  
	 b = 3;
	 d = a + b;
	 c = a + b + c;

	 t0 = a + b;
	 t1 = t0 + c;
	 d = t1;
	 b = 3;	------------------	assignment to b makes t0 invalid
	 t2 = a + b;	-------------	recalculate a+b
	 d = t2;
	 t3 = t2 + c;
	 c = t3;
(3)
	 int a = 3,b = 2, c = 1,d;
	 dt.b = 5;
	 d = a + dt.b + c;
	 d = a + dt.b;
	 dt.b = 6;
	 d = a + dt.b;

	dt[4] = 5;
	t1 = a + dt[4];---------------	See CreateOffset()
								SK_Offset is addressed.
	t2 = t1 + c;
	d = t2;
	t3 = a + dt[4];
	d = t3;
	dt[4] = 6;
	t4 = a + dt[4];
	d = t4;	 
(4)
	int a = 1, b = 2 , c = 3, d;
	int * ptr = &b;	----------------- b is addressed, so we had better not reuse the 
									  common subexpression containing b.
									 	We have to create a new temporary for conservative considering.
	d = a + b + c;	
	*ptr = 30;
	d = a + b;
	c = a + b + c;	

	t0 = &b;
	ptr = t0;
	t1 = a + b;
	t2 = t1 + c;
	d = t2;
	*ptr = 30;
	t5 = a + b;
	d = t5;
	t6 = a + b;
	t7 = t6 + c;
	c = t7;	
But:
	There are some temporaries are used across basic blocks.
	See examples/cfg/crossBB.c
 ************************************************************/
Symbol TryAddValue(Type ty, int op, Symbol src1, Symbol src2)
{
	int h = ((unsigned)src1 + (unsigned)src2 + op) & 15;
	ValueDef def = FSYM->valNumTable[h];
	Symbol t;
	/*******************************************************
		OP_CAST is used in AST,
		CVTI4F4 is used in uil.	Here, @op is something in opcode.h
		CVTI4F4 is done in TranslateCast(), not here.
	 *******************************************************/

	assert(op != CVTI4F4 && op != CALL && op != MOV);

	/**********************************************************
		struct Data{
			int val;
		};
		int a;
		int * ptr = &a;	// a  is addressed.
		Data dt;
		dt.val = 3;		------		SK_Offset is addressed.
	 ***********************************************************/
	if (op != ADDR && (src1->addressed || (src2 && src2->addressed)))
		goto new_temp;
	/*****************************************************************
		Search for the temorary defintion : src1 op src2 in hashtable.
	 *****************************************************************/
	while (def)
	{
		if (def->op == op && (def->src1 == src1 && def->src2 == src2))
			break;
		def = def->link;
	}
	/****************************************************
		def:		if we find a def
		def->dst != NULL	: 	it is a valid definition
			The reason for "def->dst == NULL" is we set it to NULL
				in function TrackValueChange().
		def->ownBB == CurrentBB
				if(a){
					a = 10;
					c = a + b;	----------(1)
				}else{
					d = a + b;	----------(2)
				}
				//	(1) and (2) are in different basic block
	 ****************************************************/
	if (def && def->ownBB == CurrentBB && def->dst != NULL)
		return def->dst;

new_temp:
	t = CreateTemp(ty);
	GenerateAssign(ty, t, op, src1, src2);

	def = AsVar(t)->def;
	def->link = FSYM->valNumTable[h];
	FSYM->valNumTable[h] = def;
	return t;
}
