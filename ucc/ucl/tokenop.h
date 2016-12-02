#ifndef TOKENOP
#error "You must define TOKENOP macro before include this file"
#endif

TOKENOP(TK_ASSIGN,        OP_ASSIGN,        OP_NONE)
TOKENOP(TK_BITOR_ASSIGN,  OP_BITOR_ASSIGN,  OP_NONE)
TOKENOP(TK_BITXOR_ASSIGN, OP_BITXOR_ASSIGN, OP_NONE)
TOKENOP(TK_BITAND_ASSIGN, OP_BITAND_ASSIGN, OP_NONE)
TOKENOP(TK_LSHIFT_ASSIGN, OP_LSHIFT_ASSIGN, OP_NONE)
TOKENOP(TK_RSHIFT_ASSIGN, OP_RSHIFT_ASSIGN, OP_NONE)
TOKENOP(TK_ADD_ASSIGN,    OP_ADD_ASSIGN,    OP_NONE)
TOKENOP(TK_SUB_ASSIGN,    OP_SUB_ASSIGN,    OP_NONE)
TOKENOP(TK_MUL_ASSIGN,    OP_MUL_ASSIGN,    OP_NONE)
TOKENOP(TK_DIV_ASSIGN,    OP_DIV_ASSIGN,    OP_NONE)
TOKENOP(TK_MOD_ASSIGN,    OP_MOD_ASSIGN,    OP_NONE)


TOKENOP(TK_OR,            OP_OR,            OP_NONE)
TOKENOP(TK_AND,           OP_AND,           OP_NONE)
TOKENOP(TK_BITOR,         OP_BITOR,         OP_NONE)
TOKENOP(TK_BITXOR,        OP_BITXOR,        OP_NONE)
//	TK_BITAND		used as binary		OP_BITAND		a|b
//					used as unary		OP_ADDRESS	&a
TOKENOP(TK_BITAND,        OP_BITAND,        OP_ADDRESS)
TOKENOP(TK_EQUAL,         OP_EQUAL,         OP_NONE)
TOKENOP(TK_UNEQUAL,       OP_UNEQUAL,       OP_NONE)
TOKENOP(TK_GREAT,         OP_GREAT,         OP_NONE)
TOKENOP(TK_LESS,          OP_LESS,          OP_NONE)
#if 0		// commented
TOKENOP(TK_LESS_EQ,       OP_GREAT_EQ,      OP_NONE)
TOKENOP(TK_GREAT_EQ,      OP_LESS_EQ,       OP_NONE)
#endif
#if 1		// modified
TOKENOP(TK_GREAT_EQ,     OP_GREAT_EQ ,       OP_NONE)
TOKENOP(TK_LESS_EQ, 	  OP_LESS_EQ,		OP_NONE)
#endif
TOKENOP(TK_LSHIFT,        OP_LSHIFT,        OP_NONE)
TOKENOP(TK_RSHIFT,        OP_RSHIFT,        OP_NONE)
//TK_ADD		used as binary		OP_ADD	a+b
//				used as unary		OP_POS		+a		positive
TOKENOP(TK_ADD,           OP_ADD,           OP_POS)
TOKENOP(TK_SUB,           OP_SUB,           OP_NEG)
TOKENOP(TK_MUL,           OP_MUL,           OP_DEREF)
TOKENOP(TK_DIV,           OP_DIV,           OP_NONE)
TOKENOP(TK_MOD,           OP_MOD,           OP_NONE)



TOKENOP(TK_INC,           OP_NONE,          OP_PREINC)
TOKENOP(TK_DEC,           OP_NONE,          OP_PREDEC)
TOKENOP(TK_NOT,           OP_NONE,          OP_NOT)
TOKENOP(TK_COMP,          OP_NONE,          OP_COMP)

