#ifndef __AST_H_
#define __AST_H_
/******************************************************
	NK:		Node Kind
 ******************************************************/
enum nodeKind 
{ 
	NK_TranslationUnit,     NK_Function,           NK_Declaration,
	NK_TypeName,            NK_Specifiers,         NK_Token,				
	NK_TypedefName,         NK_EnumSpecifier,      NK_Enumerator,			
	NK_StructSpecifier,     NK_UnionSpecifier,     NK_StructDeclaration,	
	NK_StructDeclarator,    NK_PointerDeclarator,  NK_ArrayDeclarator,		
	NK_FunctionDeclarator,  NK_ParameterTypeList,  NK_ParameterDeclaration,
	NK_NameDeclarator,      NK_InitDeclarator,     NK_Initializer,
	
	NK_Expression,

	NK_ExpressionStatement, NK_LabelStatement,     NK_CaseStatement,		
	NK_DefaultStatement,    NK_IfStatement,        NK_SwitchStatement,		
	NK_WhileStatement,      NK_DoStatement,        NK_ForStatement,		
	NK_GotoStatement,       NK_BreakStatement,     NK_ContinueStatement,		
	NK_ReturnStatement,     NK_CompoundStatement
};
/********************************************
	kind:	the kind of node
	next:	pointer to next node
	coord:	the coordinate of node
 ********************************************/
#define AST_NODE_COMMON   \
    int kind;             \
    struct astNode *next; \
    struct coord coord;

typedef struct astNode
{
	AST_NODE_COMMON
} *AstNode;
/***********************************************
	coord:		label's coordinate
	id:			label's name
	// In C, the scope of label is whole function.
	// The label's reference can appear before label's definition.
	ref:			label's reference count
	defined:	if label is defined
	respBB:		corresponding basic block,
				used by intermediate code generation
	next:		pointer to next label.
 ***********************************************/
typedef struct label
{
	struct coord coord;
	char *id;
	int ref;
	int defined;
	BBlock respBB;
	struct label *next;
} *Label;

typedef struct astExpression      *AstExpression;
typedef struct astStatement       *AstStatement;
typedef struct astDeclaration     *AstDeclaration;
typedef struct astTypeName        *AstTypeName;
typedef struct astTranslationUnit *AstTranslationUnit;
/********************************************************
	For example:
		struct st{
			struct{
				int a,b;
			};
			int c;
		} st = {{2},3};
	We use a initData list to represent the above initialization:	
		(offset:0, expr:2)  --> (offset:8,expr:3)
 ********************************************************/
struct initData
{
	int offset;
	AstExpression expr;
	InitData next;
};

#define CREATE_AST_NODE(p, k) \
    CALLOC(p);                \
    p->kind = NK_##k;         \
    p->coord = TokenCoord;

#define NEXT_TOKEN  CurrentToken = GetNextToken();

AstExpression      ParseExpression(void);
AstExpression      ParseConstantExpression(void);
AstExpression      ParseAssignmentExpression(void);
AstStatement       ParseCompoundStatement(void);
AstTypeName        ParseTypeName(void);
AstDeclaration     ParseDeclaration(void);
AstTranslationUnit ParseTranslationUnit(char *file);

void PostCheckTypedef(void);
void CheckTranslationUnit(AstTranslationUnit transUnit);
void Translate(AstTranslationUnit transUnit);
void EmitTranslationUnit(AstTranslationUnit transUnit);


extern int CurFileLineNo;
extern const char * CurFileName;
void Do_Expect(int tok);
void Do_SkipTo(int toks[], char *einfo);
#define	Expect	CurFileName = __FILE__, CurFileLineNo = __LINE__ , Do_Expect
#define	SkipTo	CurFileName = __FILE__, CurFileLineNo = __LINE__ , Do_SkipTo


int  CurrentTokenIn(int toks[]);
int  IsTypeName(int tok);

void DumpTranslationUnit(AstTranslationUnit transUnit);
void DAssemTranslationUnit(AstTranslationUnit transUnit);

extern int CurrentToken;
extern int FIRST_Declaration[];

#endif

