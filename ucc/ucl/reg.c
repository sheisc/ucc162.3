#include "ucl.h"
#include "gen.h"
#include "reg.h"
#include "target.h"
#include "output.h"

#include "ast.h"
#include "expr.h"
#include "input.h"


Symbol X86Regs[EDI + 1];
Symbol X86ByteRegs[EDI + 1];
Symbol X86WordRegs[EDI + 1];
/******************************************************
	see static void EmitBBlock(BBlock bb)
	We set UsedRegs to 0 before we emit assembly code 
	for every UIL instruction.
 ******************************************************/
int UsedRegs;

static int FindEmptyReg(int endr)
{
	int i;
	
	for (i = EAX; i <= endr; ++i)
	{
		/*****************************************************************
		
		if(X86Regs[i] != NULL && (1 << i & UsedRegs)){
			assert(X86Regs[i]->link != NULL);	----------- This assertion fails.
		}
		 *******************************************************************/
		if (X86Regs[i] != NULL && X86Regs[i]->link == NULL && ! (1 << i & UsedRegs))
			return i;
	}

	return NO_REG;
}

static int SelectSpillReg(int endr)
{
	Symbol p;
	int i;
	int reg = NO_REG;
	int ref, mref = INT_MAX;

	for (i = EAX; i <= endr; ++i)
	{
		/*****************************************************
			(1)	X86Regs[i] == NULL	when i is ESP or EBP
			(2)	(1 << i & UsedRegs):
				the current UIL instruction has used X86Regs[i] for one of its operands.
				We can't spill this register.
			(3)	We clear UsedRegs to zero before emit assembly code for 
				a UIL instruction in EmitBBlock();
		 *****************************************************/		
		if (X86Regs[i] == NULL || (1 << i & UsedRegs))
			continue;

		p = X86Regs[i]->link;
		ref = 0;
		// calculate the total references of current register
		while (p)
		{
			// the content of the register needs to be written back
			if (p->needwb && p->ref > 0)
			{
				ref += p->ref;
			}
			p = p->link;
		}
		// find the least referenced register to spill
		if (ref < mref)
		{
			mref = ref;
			reg = i;
		}
	}

	assert(reg != NO_REG);
	return reg;
}

static Symbol GetRegInternal(int width)
{
	int i, endr;
	Symbol *regs;

	switch (width)
	{
	case 1:		//  al, cl ,dl
		endr = EDX;
		regs = X86ByteRegs;
		break;

	case 2:		// AX, CX, DX, BX, SP, BP, SI, DI
		endr = EDI;
		regs = X86WordRegs;
		break;

	case 4:		// EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI
		endr = EDI;
		regs = X86Regs;
		break;
	}
	// try to find an unused register, that is , empty
	i = FindEmptyReg(endr);
	// If there is no empty register
	if (i == NO_REG)
	{
		// spill the content of the selected register to memory
		i = SelectSpillReg(endr);
		SpillReg(X86Regs[i]);
	}
	//  If AL is used, EAX is used.  Vice versa.
	UsedRegs |= 1 << i;

	return regs[i];
}


/********************************************************************
#define	b(i)	(b[i]+b[i+1])
 int main(){
	 int b[11];
	 int a = b(1) + (b(2) + (b(3) + (b(4) + (b(5) + (b(6)
						  + (b(7)+ (b(8)+ (b(9) + (b(0)+0)+b(1))+b(2))
								 +b(3))+b(4))+b(5))+b(6))+b(7))+b(8));
	 return 0;
 }
 ********************************************************************/
void SpillReg(Symbol reg)
{
	Symbol p;
	/**********************************
		If some variable have the same content,
		their contents may be in the same register.
		When we spill a register, 
		we should write back all these variables.
		But it seems there is at most one element in the link-list
	 **********************************/
	p = reg->link;
	while (p)
	{
		// After writing back, variable is not in register anymore.
		p->reg = NULL;
		if (p->needwb && p->ref > 0)
		{
			// After it is synchronized, set needwb to 0.
			p->needwb = 0;		
			/***************************************************
				we are sure that p is a SK_Temp,
				because it may be reused as a common sub-expression,
				we have to allocate temporary momory for it .
			 ***************************************************/
			StoreVar(reg, p);
		}
		p = p->link;
	}
	reg->link = NULL;
}
/************************************************
	When we are leaving a basic block,
	this function is calling to spill all the registers .
 ************************************************/
void ClearRegs(void)
{
	int i;

	for (i = EAX; i <= EDI; ++i)
	{
		if (X86Regs[i])
			SpillReg(X86Regs[i]);
	}
}

Symbol GetByteReg(void)
{
	return GetRegInternal(1);
}

Symbol GetWordReg(void)
{
	return GetRegInternal(2);
}

Symbol GetReg(void)
{
	return GetRegInternal(4);
}
/************************************************
for example:
	X86Regs[EBX] = CreateReg("%ebx", "(%ebx)", EBX);
	X86Regs[EAX] = CreateReg("eax", "[eax]", EAX);
	X86WordRegs[EAX] = CreateReg("%ax", NULL, EAX);
@name			"%ebx"
@iname			"(%ebx)"		indirect addressing name
@no			EAX		EBX		...
 ************************************************/
Symbol CreateReg(char *name, char *iname, int no)
{
	Symbol reg;

	CALLOC(reg);

	reg->kind = SK_Register;
	reg->name = reg->aname = name;
	reg->val.i[0] = no;
	reg->reg = reg;

	if (iname != NULL)
	{
		CALLOC(reg->next);
		reg->next->kind = SK_IRegister;
		reg->next->name = reg->next->aname = iname;
	}

	return reg;
}

