#ifndef __GRAMMER_H_
#define __GRAMMER_H_
//  tokens that may be first token in a declaration
#define FIRST_DECLARATION                                           \
    TK_AUTO,   TK_EXTERN,   TK_REGISTER, TK_STATIC,   TK_TYPEDEF,   \
    TK_CONST,  TK_VOLATILE, TK_SIGNED,   TK_UNSIGNED, TK_SHORT,     \
    TK_LONG,   TK_CHAR,     TK_INT,      TK_INT64,    TK_FLOAT,	    \
    TK_DOUBLE, TK_ENUM,     TK_STRUCT,   TK_UNION,    TK_VOID, TK_ID
// fisrt token of an expression
#define FIRST_EXPRESSION                                                          \
    TK_SIZEOF,       TK_ID,         TK_INTCONST,    TK_UINTCONST,  TK_LONGCONST,  \
    TK_ULONGCONST,   TK_LLONGCONST, TK_ULLONGCONST, TK_FLOATCONST, TK_DOUBLECONST,\
    TK_LDOUBLECONST, TK_STRING,     TK_WIDESTRING,  TK_BITAND,     TK_ADD,        \
    TK_SUB,          TK_MUL,        TK_INC,         TK_DEC,        TK_NOT,        \
    TK_COMP,         TK_LPAREN
// first token of statement
#define FIRST_STATEMENT                                                                   \
    TK_BREAK, TK_CASE,   TK_CONTINUE, TK_DEFAULT, TK_DO,    TK_FOR,       TK_GOTO,        \
    TK_IF,    TK_LBRACE, TK_RETURN,   TK_SWITCH,  TK_WHILE, TK_SEMICOLON, FIRST_EXPRESSION

#endif


