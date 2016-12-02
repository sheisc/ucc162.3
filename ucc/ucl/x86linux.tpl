#ifndef TEMPLATE
#error "You must define TEMPLATE macro before include this file"
#endif



TEMPLATE(X86_BORI4,     "orl %2, %0")
TEMPLATE(X86_BORU4,     "orl %2, %0")
TEMPLATE(X86_BORF4,     NULL)
TEMPLATE(X86_BORF8,     NULL)

TEMPLATE(X86_BXORI4,    "xorl %2, %0")
TEMPLATE(X86_BXORU4,    "xorl %2, %0")
TEMPLATE(X86_BXORF4,    NULL)
TEMPLATE(X86_BXORF8,    NULL)

TEMPLATE(X86_BANDI4,    "andl %2, %0")
TEMPLATE(X86_BANDU4,    "andl %2, %0")
TEMPLATE(X86_BANDF4,    NULL)
TEMPLATE(X86_BANDF8,    NULL)

TEMPLATE(X86_LSHI4,    "shll %2, %0")
TEMPLATE(X86_LSHU4,    "shll %2, %0")
TEMPLATE(X86_LSHF4,    NULL)
TEMPLATE(X86_LSHF8,    NULL)

TEMPLATE(X86_RSHI4,    "sarl %2, %0")
TEMPLATE(X86_RSHU4,    "shrl %2, %0")
TEMPLATE(X86_RSHF4,    NULL)
TEMPLATE(X86_RSHF8,    NULL)

TEMPLATE(X86_ADDI4,    "addl %2, %0")
TEMPLATE(X86_ADDU4,    "addl %2, %0")
TEMPLATE(X86_ADDF4,    "fadds %2")
TEMPLATE(X86_ADDF8,    "faddl %2")

TEMPLATE(X86_SUBI4,    "subl %2, %0")
TEMPLATE(X86_SUBU4,    "subl %2, %0")
TEMPLATE(X86_SUBF4,    "fsubs %2")
TEMPLATE(X86_SUBF8,    "fsubl %2")

TEMPLATE(X86_MULI4,    "imull %2, %0")
TEMPLATE(X86_MULU4,    "mull %2")
TEMPLATE(X86_MULF4,    "fmuls %2")
TEMPLATE(X86_MULF8,    "fmull %2")

TEMPLATE(X86_DIVI4,    "cdq;idivl %2")
TEMPLATE(X86_DIVU4,    "movl $0, %%edx;divl %2")
TEMPLATE(X86_DIVF4,    "fdivs %2")
TEMPLATE(X86_DIVF8,    "fdivl %2")

TEMPLATE(X86_MODI4,    "cdq;idivl %2")
TEMPLATE(X86_MODU4,    "movl $0, %%edx; divl %2")
TEMPLATE(X86_MODF4,    NULL)
TEMPLATE(X86_MODF8,    NULL)

TEMPLATE(X86_NEGI4,    "negl %0")
TEMPLATE(X86_NEGU4,    "negl %0")
TEMPLATE(X86_NEGF4,    "fchs")
TEMPLATE(X86_NEGF8,    "fchs")

TEMPLATE(X86_COMPI4,   "notl %0")
TEMPLATE(X86_COMPU4,   "notl %0")
TEMPLATE(X86_COMPF4,   NULL)
TEMPLATE(X86_COMPF8,   NULL)

TEMPLATE(X86_JZI4,     "cmpl $0, %1;je %0")
TEMPLATE(X86_JZU4,     "cmpl $0, %1;je %0")
TEMPLATE(X86_JZF4,     "fldz;fucompp;fnstsw %%ax;test $0x44, %%ah;jnp %0")
TEMPLATE(X86_JZF8,     "fldz;fucompp;fnstsw %%ax;test $0x44, %%ah;jnp %0")

TEMPLATE(X86_JNZI4,    "cmpl $0, %1;jne %0")
TEMPLATE(X86_JNZU4,    "cmpl $0, %1;jne %0")
TEMPLATE(X86_JNZF4,    "fldz;fucompp;fnstsw %%ax;test $0x44, %%ah;jp %0")
TEMPLATE(X86_JNZF8,    "fldz;fucompp;fnstsw %%ax;test $0x44, %%ah;jp %0")

TEMPLATE(X86_JEI4,     "cmpl %2, %1;je %0")
TEMPLATE(X86_JEU4,     "cmpl %2, %1;je %0")
TEMPLATE(X86_JEF4,     "flds %2;fucompp;fnstsw %%ax;test $0x44, %%ah;jnp %0")
TEMPLATE(X86_JEF8,     "fldl %2;fucompp;fnstsw %%ax;test $0x44, %%ah;jnp %0")

TEMPLATE(X86_JNEI4,    "cmpl %2, %1;jne %0")
TEMPLATE(X86_JNEU4,    "cmpl %2, %1;jne %0")
TEMPLATE(X86_JNEF4,    "flds %2;fucompp;fnstsw %%ax;test $0x44, %%ah;jp %0")
TEMPLATE(X86_JNEF8,    "fldl %2;fucompp;fnstsw %%ax;test $0x44, %%ah;jp %0")



TEMPLATE(X86_JGI4,     "cmpl %2, %1;jg %0")
TEMPLATE(X86_JGU4,     "cmpl %2, %1;ja %0")
TEMPLATE(X86_JGF4,     "flds %2;fucompp;fnstsw %%ax;test $0x1, %%ah;jne %0")
TEMPLATE(X86_JGF8,     "fldl %2;fucompp;fnstsw %%ax;test $0x1, %%ah;jne %0")

TEMPLATE(X86_JLI4,     "cmpl %2, %1;jl %0")
TEMPLATE(X86_JLU4,     "cmpl %2, %1;jb %0")
TEMPLATE(X86_JLF4,     "flds %2;fucompp;fnstsw %%ax;test $0x41, %%ah;jp %0")
TEMPLATE(X86_JLF8,     "fldl %2;fucompp;fnstsw %%ax;test $0x41, %%ah;jp %0")

TEMPLATE(X86_JGEI4,    "cmpl %2, %1;jge %0")
TEMPLATE(X86_JGEU4,    "cmpl %2, %1;jae %0")
TEMPLATE(X86_JGEF4,    "flds %2;fucompp;fnstsw %%ax;test $0x41, %%ah;jne %0")
TEMPLATE(X86_JGEF8,    "fldl %2;fucompp;fnstsw %%ax;test $0x41, %%ah;jne %0")

TEMPLATE(X86_JLEI4,    "cmpl %2, %1;jle %0")
TEMPLATE(X86_JLEU4,    "cmpl %2, %1;jbe %0")
TEMPLATE(X86_JLEF4,    "flds %2;fucompp;fnstsw %%ax;test $0x5, %%ah;jp %0")
TEMPLATE(X86_JLEF8,    "fldl %2;fucompp;fnstsw %%ax;test $0x5, %%ah;jp %0")



TEMPLATE(X86_EXTI1,    "movsbl %1, %0")
TEMPLATE(X86_EXTU1,    "movzbl %1, %0")
TEMPLATE(X86_EXTI2,    "movswl %1, %0")
TEMPLATE(X86_EXTU2,    "movzwl %1, %0")
TEMPLATE(X86_TRUI1,    "movb %%al, %0")
TEMPLATE(X86_TRUI2,    "movb %%al, %0")

 
TEMPLATE(X86_CVTI4F4,  "pushl %1;fildl (%%esp);fstps %0;addl $4, %%esp")
TEMPLATE(X86_CVTI4F8,  "pushl %1;fildl (%%esp);fstpl %0;addl $4, %%esp")
TEMPLATE(X86_CVTU4F4,  "pushl $0;pushl %1;fildq (%%esp);fstps %0;addl $8, %%esp")
TEMPLATE(X86_CVTU4F8,  "pushl $0;pushl %1;fildq (%%esp);fstpl %0;addl $8, %%esp")

TEMPLATE(X86_CVTF4,    "flds %1;fstpl %0") 

 
TEMPLATE(X86_CVTF4I4,  "flds %1;subl $16, %%esp;fnstcw (%%esp);movzwl (%%esp), %%eax;"
                       "orl $0x0c00, %%eax;movl %%eax, 4(%%esp);fldcw 4(%%esp);fistpl 8(%%esp);"
                       "fldcw (%%esp);movl 8(%%esp), %%eax;addl $16, %%esp")
 
TEMPLATE(X86_CVTF4U4,  "flds %1;subl $16, %%esp;fnstcw (%%esp);movzwl (%%esp), %%eax;"
                       "orl $0x0c00, %%eax;movl %%eax, 4(%%esp);fldcw 4(%%esp);fistpll 8(%%esp);"
                       "fldcw (%%esp);movl 8(%%esp), %%eax;addl $16, %%esp")
TEMPLATE(X86_CVTF8,    "fldl %1;fstps %0")

TEMPLATE(X86_CVTF8I4,  "fldl %1;subl $16, %%esp;fnstcw (%%esp);movzwl (%%esp), %%eax;"
                       "orl $0x0c00, %%eax;movl %%eax, 4(%%esp);fldcw 4(%%esp);fistpl 8(%%esp);"
                       "fldcw (%%esp);movl 8(%%esp), %%eax; addl $16, %%esp")
 
TEMPLATE(X86_CVTF8U4,  "fldl %1;subl $16, %%esp;fnstcw (%%esp);movzwl (%%esp), %%eax;"
                       "orl $0x0c00, %%eax;movl %%eax, 4(%%esp);fldcw 4(%%esp);fistpll 8(%%esp);"
                       "fldcw (%%esp);movl 8(%%esp), %%eax;addl $16, %%esp")
					
		
TEMPLATE(X86_INCI1,    "incb %0")
TEMPLATE(X86_INCU1,    "incb %0")
TEMPLATE(X86_INCI2,    "incw %0")
TEMPLATE(X86_INCU2,    "incw %0")				
TEMPLATE(X86_INCI4,    "incl %0")
TEMPLATE(X86_INCU4,    "incl %0")
TEMPLATE(X86_INCF4,    "fld1;fadds %0;fstps %0")

 
TEMPLATE(X86_INCF8,    "fld1;faddl %0;fstpl %0")

TEMPLATE(X86_DECI1,    "decb %0")
TEMPLATE(X86_DECU1,    "decb %0")
TEMPLATE(X86_DECI2,    "decw %0")
TEMPLATE(X86_DECU2,    "decw %0")
TEMPLATE(X86_DECI4,    "decl %0")
TEMPLATE(X86_DECU4,    "decl %0")

TEMPLATE(X86_DECF4,    "fld1;fsubs %0;fchs;fstps %0")
TEMPLATE(X86_DECF8,    "fld1;fsubl %0;fchs;fstpl %0")

TEMPLATE(X86_ADDR,     "leal %1, %0")

TEMPLATE(X86_MOVI1,    "movb %1, %0")
TEMPLATE(X86_MOVI2,    "movw %1, %0")
TEMPLATE(X86_MOVI4,    "movl %1, %0")
TEMPLATE(X86_MOVB,     "leal %0, %%edi;leal %1, %%esi;movl %2, %%ecx;rep movsb")


TEMPLATE(X86_JMP,      "jmp %0")
TEMPLATE(X86_IJMP,     "jmp *%0(,%1,4)")

TEMPLATE(X86_PROLOGUE, "pushl %%ebp;pushl %%ebx;pushl %%esi;pushl %%edi;movl %%esp, %%ebp")
TEMPLATE(X86_PUSH,     "pushl %0")
TEMPLATE(X86_PUSHF4,   "pushl %%ecx;flds %0;fstps (%%esp)")
TEMPLATE(X86_PUSHF8,   "subl $8, %%esp;fldl %0;fstpl (%%esp)")
TEMPLATE(X86_PUSHB,    "leal %0, %%esi;subl %2, %%esp;movl %%esp, %%edi;movl %1, %%ecx;rep movsb")
TEMPLATE(X86_EXPANDF,  "subl %0, %%esp")
TEMPLATE(X86_CALL,     "call %1")
TEMPLATE(X86_ICALL,    "call *%1")
TEMPLATE(X86_REDUCEF,  "addl %0, %%esp")
TEMPLATE(X86_EPILOGUE, "movl %%ebp, %%esp;popl %%edi;popl %%esi;popl %%ebx;popl %%ebp;ret")


TEMPLATE(X86_CLEAR,    "pushl %1;pushl $0;leal %0, %%eax;pushl %%eax;call memset;addl $12, %%esp")

TEMPLATE(X86_LDF4,     "flds %0")
TEMPLATE(X86_LDF8,     "fldl %0")
TEMPLATE(X86_STF4,     "fstps %0")
TEMPLATE(X86_STF8,     "fstpl %0")


TEMPLATE(X86_STF4_NO_POP,     "fsts %0")
TEMPLATE(X86_STF8_NO_POP,     "fstl %0")

TEMPLATE(X86_X87_POP,     "fstp %%st(0)")