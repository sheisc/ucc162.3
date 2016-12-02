#include "ucl.h"
#include "gen.h"

/**
 * Add a predecessor p to basic block bb
 */
static void AddPredecessor(BBlock bb, BBlock p)
{
	CFGEdge e;

	ALLOC(e);
	e->bb = p;
	e->next = bb->preds;
	bb->preds = e;
	bb->npred++;
}

/**
 * Add a successor s to basic block bb
 */ 
static void AddSuccessor(BBlock bb, BBlock s)
{
	CFGEdge e;

	ALLOC(e);
	e->bb = s;
	e->next = bb->succs;
	bb->succs = e;
	bb->nsucc++;
}

/**
 * Remove a basic block from the edges
 */
static void RemoveEdge(CFGEdge *pprev, BBlock bb)
{
	CFGEdge e;

	e = *pprev;
	while (e)
	{
		if (e->bb == bb)
		{
			*pprev = e->next;
			return;
		}
		pprev = &e->next;
		e = e->next;
	}
	assert(0);
	
}

/**
 * Remove a predecessor p from basic block bb
 */
static void RemovePredecessor(BBlock bb, BBlock p)
{
	RemoveEdge(&bb->preds, p);
	bb->npred--;
}

/**
 * Remove a successor s from basic block bb
 */
static void RemoveSuccessor(BBlock bb, BBlock s)
{
	RemoveEdge(&bb->succs, s);
	bb->nsucc--;
}

/**
 * If p is not a predecessor of basic block bb, 
 * add the predecessor p to bb
 */
static void TryAddPredecessor(BBlock bb, BBlock p)
{
	CFGEdge pred;

	pred = bb->preds;
	while (pred != NULL)
	{
		if (pred->bb == p)
			return;
		pred = pred->next;
	}
	AddPredecessor(bb, p);
}

/**
 * If s is not a predecessor of basic block bb,
 * add the successor s to bb
 */
static void TryAddSuccessor(BBlock bb, BBlock s)
{
	CFGEdge succ;

	succ = bb->succs;
	while (succ != NULL)
	{
		if (succ->bb == s)
			return;
		succ = succ->next;
	}
	AddSuccessor(bb, s);
}

/**
 * Merge two basic blocks bb1 and bb2 into one basic block bb1.
 */
static void MergeInstructions(BBlock bb1, BBlock bb2)
{
	IRInst bb1_lasti = bb1->insth.prev;
	IRInst bb2_firsti = bb2->insth.next;
	
	if (bb1_lasti->opcode >= JZ && bb1_lasti->opcode <= JMP)
	{
		bb1_lasti = bb1_lasti->prev;
		bb1->ninst--;
	}

	if (bb2->ninst == 0)
	{
		bb1->insth.prev = bb1_lasti;
		bb1_lasti->next = &bb1->insth;
	}
	else
	{
		bb1_lasti->next = bb2_firsti;
		bb2_firsti->prev = bb1_lasti;
		bb2->insth.prev->next = &bb1->insth;
		bb1->insth.prev = bb2->insth.prev;
	}
	bb1->ninst += bb2->ninst;
}

static void ModifySuccessor(BBlock bb, BBlock s, BBlock ns)
{
	IRInst lasti = bb->insth.prev;

	RemoveSuccessor(bb, s);
	TryAddSuccessor(bb, ns);
	TryAddPredecessor(ns, bb);
	if (lasti->opcode >= JZ && lasti->opcode <= JMP)
	{
		if ((BBlock)lasti->opds[0] == s)
		{
			s->ref--;
			ns->ref++;			
			lasti->opds[0] = (Symbol)ns;
		}
	}
	else if (lasti->opcode == IJMP)
	{
		BBlock *dstBBs = (BBlock *)lasti->opds[0];
		int i = 0;

		while (dstBBs[i] != NULL)
		{
			if (dstBBs[i] == s)
			{
				s->ref--;
				ns->ref++;
				dstBBs[i] = ns;
			}
			i++;
		}
	}
}
/******************************************
		head 		tail

	after DrawCFGEdge(...)

		head	--->	tail
		
	In fact , we just record that head is a predecessor of tail,
			tail is a successor of head.
 ******************************************/
void DrawCFGEdge(BBlock head, BBlock tail)
{
	AddSuccessor(head, tail);
	AddPredecessor(tail, head);
}

BBlock TryMergeBBlock(BBlock bb1, BBlock bb2)
{
	if (bb2 == NULL)
		return bb2;
	if (bb1->nsucc == 1 && bb2->npred == 1 && bb1->succs->bb == bb2)
	{
		CFGEdge succ;		

		if(bb2->ref  >  1){
			IRInst lasti = bb1->insth.prev;
			if (lasti->opcode == IJMP ){				
				lasti->prev->next = lasti->next;
				lasti->next->prev = lasti->prev;
				bb1->ninst--;
			}
		}

		succ = bb2->succs;
		while (succ != NULL)
		{
			RemovePredecessor(succ->bb, bb2);
			AddPredecessor(succ->bb, bb1);
			succ = succ->next;
		}
		bb1->succs = bb2->succs;
		bb1->nsucc = bb2->nsucc;
		MergeInstructions(bb1, bb2);
		bb1->next = bb2->next;
		if (bb2->next)
			bb2->next->prev = bb1;

		return bb1;
	}
	else if (bb1->ninst == 0)
	{
		CFGEdge pred;

		RemovePredecessor(bb2, bb1);
		pred = bb1->preds;
		while (pred != NULL)
		{
			ModifySuccessor(pred->bb, bb1, bb2);
			pred = pred->next;
		}
		bb2->prev = bb1->prev;
		if (bb1->prev)
			bb1->prev->next = bb2;
		
		return bb1->prev;
	}
	else if (bb2->ninst == 0 && bb2->npred == 0)
	{
		if (bb2->next == NULL)
		{
			bb1->next = NULL;
		}
		else
		{
			RemovePredecessor(bb2->next, bb2);
			bb1->next = bb2->next;
			bb2->next->prev = bb1;
		}

		return bb1;
	}
	else if (bb2->ninst == 0 && bb2->npred == 1 && bb2->preds->bb == bb1)
	{
		if (bb2->next == NULL)
		{
			bb1->next = NULL;
		}
		else
		{
			RemovePredecessor(bb2->next, bb2);
			ModifySuccessor(bb1, bb2, bb2->next);
			bb1->next = bb2->next;
			bb2->next->prev = bb1;
		}

		return bb1;
	}
	else
	{
		IRInst lasti = bb1->insth.prev;
		if (lasti->opcode == JMP && bb1->succs->bb == bb1->next)
		{
			lasti->prev->next = lasti->next;
			lasti->next->prev = lasti->prev;
			bb1->ninst--;
			( (BBlock) (lasti->opds[0]))->ref--;
			return bb1;
		}		
	}
	
	return bb2;
}

void ExamineJump(BBlock bb)
{
	IRInst lasti;
	CFGEdge succ;
	BBlock bb1, bb2;

	lasti = bb->insth.prev;
	/**************************************************
	 OPCODE(JZ, 	 "",					 Branch)
	 OPCODE(JNZ,	 "!",					 Branch)
	 OPCODE(JE, 	 "==",					 Branch)
	 OPCODE(JNE,	 "!=",					 Branch)
	 OPCODE(JG, 	 ">",					 Branch)
	 OPCODE(JL, 	 "<",					 Branch)
	 OPCODE(JGE,	 ">=",					 Branch)
	 OPCODE(JLE,	 "<=",					 Branch)
	 OPCODE(JMP,	 "jmp", 				 Jump)
	 **************************************************/
	if (! (lasti->opcode >= JZ && lasti->opcode <= JMP))
		return;

	/**
	 * jump to jump             conditional jump to jump
	 *
	 * jmp bb1                  if a < b jmp bb1
	 * ...                      ...
	 * bb1: jmp bb2             bb1: jmp bb2
	 *
	 */

	succ = bb->succs;
	do
	{
		if (succ->bb == (BBlock)lasti->opds[0])
			break;
		succ = succ->next;
	} while (succ != NULL);

	bb1 = succ->bb;
	if (bb1->ninst == 1 && bb1->insth.prev->opcode == JMP)
	{
		bb2 = bb1->succs->bb;
		succ->bb = bb2;
		RemovePredecessor(bb1, bb);
		AddPredecessor(bb2, bb);
		bb1->ref--;
		bb2->ref++;
		lasti->opds[0] = (Symbol)bb2;
	}
}



