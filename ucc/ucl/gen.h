#ifndef __GEN_H_
#define __GEN_H_
/*********************************************
	 OPCODE(INC,	 "++",					 Inc)
	 OPCODE(DEC,	 "--",					 Dec)
	 OPCODE(ADDR,	 "&",					 Address)
	 OPCODE(DEREF,	 "*",					 Deref)
	 OPCODE(EXTI1,	 "(int)(char)", 		 Cast)
 *********************************************/
enum OPCode
{
#define OPCODE(code, name, func) code, 
#include "opcode.h" 
#undef OPCODE
};
/****************************************
//Intermediate Representation Instruction
see uildasm.c

#define DST  inst->opds[0]
#define SRC1 inst->opds[1]
#define SRC2 inst->opds[2]
	prev:	pointer to previous instruction
	next:	pointer to next instruction
	ty:		instruction operating type
	opcode:	operation code
	opds:	operands, at most three
 *****************************************/
typedef struct irinst
{
	struct irinst *prev;
	struct irinst *next;
	Type ty;
	int opcode;
	Symbol opds[3];
} *IRInst;
// control flow graph edge
typedef struct cfgedge
{
	BBlock bb;
	struct cfgedge *next;
} *CFGEdge;
/**************************************************
	basic block definition:
		prev:	pointer to previous basic block
		next:	pointer to next basic block
		sym:	symbol to represent basic block
		succs:	all the successors of this basic block
		preds:	all the predecessors of this basic block
		insth:	instruction list
		ninst:	number of instructions
		nsucc:	number of successors
		npred:	number of predecessors
 ***************************************************/
struct bblock
{
	struct bblock *prev;
	struct bblock *next;
	Symbol sym;
	// successors
	CFGEdge succs;
	// predecessors
	CFGEdge preds;
	struct irinst insth;
	// number of instructions
	int ninst;
	// number of successors
	int nsucc;
	// number of predecessors
	int npred;
	int ref;
	#if 0		// commented
	//int no;
	#endif
};

typedef struct ilarg
{
	Symbol sym;
	Type ty;
} *ILArg;

BBlock CreateBBlock(void);
void   StartBBlock(BBlock bb);

void GenerateMove(Type ty, Symbol dst, Symbol src);
void GenerateIndirectMove(Type ty, Symbol dst, Symbol src);
void GenerateAssign(Type ty, Symbol dst, int opcode, Symbol src1, Symbol src2);
void GenerateInc(Type ty, Symbol src);
void GenerateDec(Type ty, Symbol src);
void GenerateBranch(Type ty, BBlock dstBB, int opcode, Symbol src1, Symbol src2);
void GenerateJump(BBlock dstBB);
void GenerateIndirectJump(BBlock *dstBBs, int len, Symbol index);
void GenerateReturn(Type ty, Symbol src);
void GenerateFunctionCall(Type ty, Symbol recv, Symbol faddr, Vector args);
void GenerateClear(Symbol dst, int size);

void DefineTemp(Symbol t, int op, Symbol src1, Symbol src2);
Symbol AddressOf(Symbol sym);
Symbol Deref(Type ty, Symbol addr);
Symbol TryAddValue(Type ty, int op, Symbol src1, Symbol src2);
Symbol Simplify(Type ty, int op, Symbol src1, Symbol src2);

void DrawCFGEdge(BBlock head, BBlock tail);
void ExamineJump(BBlock bb);
BBlock TryMergeBBlock(BBlock bb1, BBlock bb2);
void Optimize(FunctionSymbol fsym);

extern BBlock CurrentBB;
extern int OPMap[];

#endif

