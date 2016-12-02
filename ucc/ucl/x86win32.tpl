#ifndef TEMPLATE
#error "You must define TEMPLATE macro before include this file"
#endif

TEMPLATE(X86_BORI4,     "or %0, %d2")
TEMPLATE(X86_BORU4,     "or %0, %d2")
TEMPLATE(X86_BORF4,     NULL)
TEMPLATE(X86_BORF8,     NULL)

TEMPLATE(X86_BXORI4,    "xor %0, %d2")
TEMPLATE(X86_BXORU4,    "xor %0, %d2")
TEMPLATE(X86_BXORF4,    NULL)
TEMPLATE(X86_BXORF8,    NULL)

TEMPLATE(X86_BANDI4,    "and %0, %d2")
TEMPLATE(X86_BANDU4,    "and %0, %d2")
TEMPLATE(X86_BANDF4,    NULL)
TEMPLATE(X86_BANDF8,    NULL)

TEMPLATE(X86_LSHI4,     "shl %0, %2")
TEMPLATE(X86_LSHU4,     "shl %0, %2")
TEMPLATE(X86_LSHF4,     NULL)
TEMPLATE(X86_LSHF8,     NULL)

TEMPLATE(X86_RSHI4,     "sar %0, %2")
TEMPLATE(X86_RSHU4,     "shr %0, %2")
TEMPLATE(X86_RSHF4,     NULL)
TEMPLATE(X86_RSHF8,     NULL)

TEMPLATE(X86_ADDI4,     "add %0, %d2")
TEMPLATE(X86_ADDU4,     "add %0, %d2")
TEMPLATE(X86_ADDF4,     "fadd DWORD PTR %2")
TEMPLATE(X86_ADDF8,     "fadd QWORD PTR %2")

TEMPLATE(X86_SUBI4,     "sub %0, %d2")
TEMPLATE(X86_SUBU4,     "sub %0, %d2")
TEMPLATE(X86_SUBF4,     "fsub DWORD PTR %2")
TEMPLATE(X86_SUBF8,     "fsub QWORD PTR %2")

TEMPLATE(X86_MULI4,     "imul %0, %d2")
TEMPLATE(X86_MULU4,     "mul %d2")
TEMPLATE(X86_MULF4,     "fmul DWORD PTR %2")
TEMPLATE(X86_MULF8,     "fmul QWORD PTR %2")

TEMPLATE(X86_DIVI4,     "cdq;idiv %d2")
TEMPLATE(X86_DIVU4,     "mov edx, 0;div %d2")
TEMPLATE(X86_DIVF4,     "fdiv DWORD PTR %2")
TEMPLATE(X86_DIVF8,     "fdiv QWORD PTR %2")

TEMPLATE(X86_MODI4,     "cdq;idiv %d2")
TEMPLATE(X86_MODU4,     "mov edx, 0;div %d2")
TEMPLATE(X86_MODF4,     NULL)
TEMPLATE(X86_MODF8,     NULL)

TEMPLATE(X86_NEGI4,     "neg %0")
TEMPLATE(X86_NEGU4,     "neg %0")
TEMPLATE(X86_NEGF4,     "fchs")
TEMPLATE(X86_NEGF8,     "fchs")

TEMPLATE(X86_BCOMI4,    "not %0")
TEMPLATE(X86_BCOMU4,    "not %0")
TEMPLATE(X86_BCOMF4,    NULL)
TEMPLATE(X86_BCOMF8,    NULL)

TEMPLATE(X86_JZI4,      "cmp %d1, 0;je %0")
TEMPLATE(X86_JZU4,      "cmp %d1, 0;je %0")
TEMPLATE(X86_JZF4,      "fldz;fucompp;fnstsw ax;test ah, 44h;jnp %0")
TEMPLATE(X86_JZF8,      "fldz;fucompp;fnstsw ax;test ah, 44h;jnp %0")

TEMPLATE(X86_JNZI4,     "cmp %d1, 0;jne %0")
TEMPLATE(X86_JNZU4,     "cmp %d1, 0;jne %0")
TEMPLATE(X86_JNZF4,     "fldz;fucompp;fnstsw ax;test ah, 44h;jp %0")
TEMPLATE(X86_JNZF8,     "fldz;fucompp;fnstsw ax;test ah, 44h;jp %0")

TEMPLATE(X86_JEI4,      "cmp %d1, %d2;je %0")
TEMPLATE(X86_JEU4,      "cmp %d1, %d2;je %0")
TEMPLATE(X86_JEF4,      "fld DWORD PTR %2;fucompp;fnstsw ax;test ah, 44h;jnp %0")
TEMPLATE(X86_JEF8,      "fld QWORD PTR %2;fucompp;fnstsw ax;test ah, 44h;jnp %0")

TEMPLATE(X86_JNEI4,     "cmp %d1, %d2;jne %0")
TEMPLATE(X86_JNEU4,     "cmp %d1, %d2;jne %0")
TEMPLATE(X86_JNEF4,     "fld DWORD PTR %2;fucompp;fnstsw ax;test ah, 44h;jp %0")
TEMPLATE(X86_JNEF8,     "fld QWORD PTR %2;fucompp;fnstsw ax;test ah, 44h;jp %0")

TEMPLATE(X86_JGI4,      "cmp %d1, %d2;jg %0")
TEMPLATE(X86_JGU4,      "cmp %d1, %d2;ja %0")
TEMPLATE(X86_JGF4,      "fld DWORD PTR %2;fucompp;fnstsw ax;test ah, 1;jne %0")
TEMPLATE(X86_JGF8,      "fld QWORD PTR %2;fucompp;fnstsw ax;test ah, 1;jne %0")

TEMPLATE(X86_JLI4,      "cmp %d1, %d2;jl %0")
TEMPLATE(X86_JLU4,      "cmp %d1, %d2;jb %0")
TEMPLATE(X86_JLF4,      "fld DWORD PTR %2;fucompp;fnstsw ax;test ah, 41h;jp %0")
TEMPLATE(X86_JLF8,      "fld QWORD PTR %2;fucompp;fnstsw ax;test ah, 41h;jp %0")

TEMPLATE(X86_JGEI4,     "cmp %d1, %d2;jge %0")
TEMPLATE(X86_JGEU4,     "cmp %d1, %d2;jae %0")
TEMPLATE(X86_JGEF4,     "fld DWORD PTR %2;fucompp;fnstsw ax;test ah, 41h;jne %0")
TEMPLATE(X86_JGEF8,     "fld QWORD PTR %2;fucompp;fnstsw ax;test ah, 41h;jne %0")

TEMPLATE(X86_JLEI4,     "cmp %d1, %d2;jle %0")
TEMPLATE(X86_JLEU4,     "cmp %d1, %d2;jbe %0")
TEMPLATE(X86_JLEF4,     "fld DWORD PTR %2;fucompp;fnstsw ax;test ah, 5;jp %0")
TEMPLATE(X86_JLEF8,     "fld QWORD PTR %2;fucompp;fnstsw ax;test ah, 5;jp %0")


TEMPLATE(X86_EXTI1,     "movsx %0, BYTE PTR %1")
TEMPLATE(X86_EXTU1,     "movzx %0, BYTE PTR %1")
TEMPLATE(X86_EXTI2,     "movsx %0, WORD PTR %1")
TEMPLATE(X86_EXTU2,     "movzx %0, WORD PTR %1")
TEMPLATE(X86_TRUI1,     "")
TEMPLATE(X86_TRUI2,     "")


TEMPLATE(X86_CVTI4F4,   "push %d1;fild DWORD PTR [esp];fstp DWORD PTR %0;add esp,4")
TEMPLATE(X86_CVTI4F8,   "push %d1;fild DWORD PTR [esp];fstp QWORD PTR %0;add esp,4")
TEMPLATE(X86_CVTU4F4,   "push 0;push %d1;fild QWORD PTR [esp];fstp DWORD PTR %0;add esp,8")
TEMPLATE(X86_CVTU4F8,   "push 0;push %d1;fild QWORD PTR [esp];fstp QWORD PTR %0;add esp,8")
TEMPLATE(X86_CVTF4,     "fld DWORD PTR %1;fstp QWORD PTR %0")


TEMPLATE(X86_CVTF4I4,   "fld DWORD PTR %1;sub esp, 16;fnstcw [esp];movzx eax, WORD PTR [esp];"
                        "or eax, 0c00H;mov 4[esp], eax;fldcw 4[esp];fistp DWORD PTR 8[esp];"
                        "fldcw [esp];mov eax, 8[esp];add esp, 16")
TEMPLATE(X86_CVTF4U4,   "fld DWORD PTR %1;sub esp, 16;fnstcw [esp];movzx eax, WORD PTR [esp];"
                        "or eax, 0c00H;mov 4[esp], eax;fldcw 4[esp];fistp QWORD PTR 8[esp];"
                        "fldcw [esp];mov eax, 8[esp];add esp, 16")
TEMPLATE(X86_CVTF8,     "fld QWORD PTR %1;fstp DWORD PTR %0")
TEMPLATE(X86_CVTF8I4,   "fld QWORD PTR %1;sub esp, 16;fnstcw [esp];movzx eax, WORD PTR [esp];"
                        "or eax, 0c00H;mov 4[esp], eax;fldcw 4[esp];fistp DWORD PTR 8[esp];"
                        "fldcw [esp];mov eax, 8[esp];add esp, 16")
TEMPLATE(X86_CVTF8U4,   "fld QWORD PTR %1;sub esp, 16;fnstcw [esp];movzx eax, WORD PTR [esp];"
                        "or eax, 0c00H;mov 4[esp], eax;fldcw 4[esp];fistp QWORD PTR 8[esp];"
                        "fldcw [esp];mov eax, 8[esp];add esp, 16")

TEMPLATE(X86_INCI1,     "inc BYTE PTR %0")
TEMPLATE(X86_INCU1,     "inc BYTE PTR %0")
TEMPLATE(X86_INCI2,     "inc WORD PTR %0")
TEMPLATE(X86_INCU2,     "inc WORD PTR %0")						
TEMPLATE(X86_INCI4,     "inc DWORD PTR %0")
TEMPLATE(X86_INCU4,     "inc DWORD PTR %0")
TEMPLATE(X86_INCF4,     "fld1;fadd DWORD PTR %0;fstp DWORD PTR %0")


TEMPLATE(X86_INCF8,     "fld1;fadd QWORD PTR %0;fstp QWORD PTR %0")

TEMPLATE(X86_DECI1,     "dec BYTE PTR %0")
TEMPLATE(X86_DECU1,     "dec BYTE PTR %0")
TEMPLATE(X86_DECI2,     "dec WORD PTR %0")
TEMPLATE(X86_DECU2,     "dec WORD PTR %0")
TEMPLATE(X86_DECI4,     "dec DWORD PTR %0")
TEMPLATE(X86_DECU4,     "dec DWORD PTR %0")


TEMPLATE(X86_DECF4,     "fld1;fsub DWORD PTR %0;fchs;fstp DWORD PTR %0")
TEMPLATE(X86_DECF8,     "fld1;fsub QWORD PTR %0;fchs;fstp QWORD PTR %0")

TEMPLATE(X86_ADDR,      "lea %0, %1")

TEMPLATE(X86_MOVI1,     "mov %b0, %b1")
TEMPLATE(X86_MOVI2,     "mov %w0, %w1")
TEMPLATE(X86_MOVI4,     "mov %d0, %d1")
TEMPLATE(X86_MOVB,      "lea edi, %0;lea esi, %1;mov ecx, %2;rep movsb")


TEMPLATE(X86_JMP,       "jmp %0")
TEMPLATE(X86_IJMP,      "jmp DWORD PTR %0[%1*4]")

TEMPLATE(X86_PROLOGUE,  "push ebp;push ebx;push esi;push edi;mov ebp, esp")
TEMPLATE(X86_PUSH,      "push %d0")
TEMPLATE(X86_PUSHF4,    "push ecx;fld DWORD PTR %0;fstp DWORD PTR [esp]")
TEMPLATE(X86_PUSHF8,    "sub esp, 8;fld QWORD PTR %0;fstp QWORD PTR [esp]")
TEMPLATE(X86_PUSHB,     "lea esi, %0;sub esp, %2;mov edi, esp;mov ecx, %1;rep movsb")
TEMPLATE(X86_EXPANDF,   "sub esp, %0")
TEMPLATE(X86_CALL,      "call %1")
TEMPLATE(X86_ICALL,     "call DWORD PTR %1")
TEMPLATE(X86_REDUCEF,   "add esp, %0")
TEMPLATE(X86_EPILOGUE,  "mov esp, ebp;pop edi;pop esi;pop ebx;pop ebp;ret")


TEMPLATE(X86_CLEAR,     "push %1;push 0;lea eax, %0;push eax;call _memset;add esp, 12")

TEMPLATE(X86_LDF4,      "fld DWORD PTR %0")
TEMPLATE(X86_LDF8,      "fld QWORD PTR %0")
TEMPLATE(X86_STF4,      "fstp DWORD PTR %0")
TEMPLATE(X86_STF8,      "fstp QWORD PTR %0")


TEMPLATE(X86_STF4_NO_POP,     "fst DWORD PTR %0")
TEMPLATE(X86_STF8_NO_POP,     "fst QWORD PTR %0")

TEMPLATE(X86_X87_POP,     "fstp st(0)")