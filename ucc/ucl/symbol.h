#ifndef __SYMBOL_H_
#define __SYMBOL_H_

#include "input.h"

// SK:  Symbol Kind ?
enum 
{ 
	SK_Tag,    SK_TypedefName, SK_EnumConstant, SK_Constant, SK_Variable, SK_Temp,
	SK_Offset, SK_String,      SK_Label,        SK_Function, SK_Register
};

#define SYM_HASH_MASK 127
/**************************************************
	sclass:	storage class,  TK_STATIC	TK_EXTERN		...
	link:	link symbols in the same hash bucket[*] item.
	needwb:	the content needs to be written back to memory
	reg:		int abc;     variable may be in register EAX/ECX,...
				reg represents the register in which var abc is ?
	aname:		access name,see GetAccessName()				
 **************************************************/
#define SYMBOL_COMMON     \
    int kind;             \
    char *name;           \
    char *aname;          \
    Type ty;              \
    int level;            \
    int sclass;           \
    int ref;              \
    int defined   : 1;    \
    int addressed : 1;    \
    int needwb    : 1;    \
    int unused    : 29;   \
    union value val;      \
    struct symbol *reg;   \
    struct symbol *link;  \
    struct symbol *next; \
    Coord pcoord;		

typedef struct bblock *BBlock;
typedef struct initData *InitData;

typedef struct symbol
{
	SYMBOL_COMMON
} *Symbol;
/***************************************************
	struct valueDef and struct valueUse
		are used for DU list .
	Here definition is assignment to a variable just like
		t1 = b + c;	//   defition of t1,  use of b and c.
					t1 is a temporary variable.
	Buf in UCC, we never track a named variable 's definition
		a  = b + c;	-----
 ***************************************************/
//  dst = src1  op 	src2,  the definition of a symbol
typedef struct valueDef
{
	Symbol dst;
	int op;
	Symbol src1;
	Symbol src2;
	BBlock ownBB;
	struct valueDef *link;
} *ValueDef;
// the use of a definition
typedef struct valueUse
{
	ValueDef def;
	struct valueUse *next;
} *ValueUse;

typedef struct variableSymbol
{
	SYMBOL_COMMON
	InitData idata;
	ValueDef def;
	ValueUse uses;
	int offset;
} *VariableSymbol;

typedef struct functionSymbol
{
	SYMBOL_COMMON
	Symbol params;
	Symbol locals;
	Symbol *lastv;
	int nbblock;
	BBlock entryBB;
	BBlock exitBB;
	ValueDef valNumTable[16];
} *FunctionSymbol;

typedef struct table
{
	Symbol *buckets;
	int level;
	struct table *outer;
} *Table;

#define AsVar(sym)  ((VariableSymbol)sym)
#define AsFunc(sym) ((FunctionSymbol)sym)

void InitSymbolTable(void);
void EnterScope(void);
void ExitScope(void);

Symbol LookupID(char *id);
Symbol LookupTag(char *id);


#define	ADD_PLACE_HOLDER		1
Symbol LookupFunctionID(char *name,int placeHolder);
Symbol AddConstant(Type ty, union value val);
Symbol AddEnumConstant(char *id, Type ty, int val,Coord pcoord);
Symbol AddTypedefName(char *id, Type ty,Coord pcoord);
Symbol AddVariable(char *id, Type ty, int sclass,Coord pcoord);
Symbol AddFunction(char *id, Type ty, int sclass,Coord pcoord);
Symbol AddTag(char *id, Type ty,Coord pcoord);
Symbol IntConstant(int i);
Symbol CreateTemp(Type ty);
Symbol CreateLabel(void);
Symbol AddString(Type ty, String str,Coord pcoord);
Symbol CreateOffset(Type ty, Symbol base, int offset,Coord pcoord);
const char * GetSymbolKind(int kind);



int IsInParameterList(void);
void EnterParameterList(void);
void LeaveParemeterList(void);
void SaveParameterListTable(void);
void RestoreParameterListTable(void);



extern int Level;
extern int LabelNum, TempNum;
extern Symbol Functions;
extern Symbol Globals;
extern Symbol Strings;
extern Symbol FloatConstants;
extern FunctionSymbol FSYM;

#endif


