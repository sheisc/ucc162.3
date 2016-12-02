#ifndef __DECL_H_
#define __DECL_H_
/**************************************************
	direct-abstract-declarator:
		without identifier
	direct-declarator:
		with identifier		
 **************************************************/
enum { DEC_ABSTRACT = 0x01, DEC_CONCRETE = 0x02};
// The possble type constructor, used in struct typeDerivList.
// TypeDerivList->ctor:	 POINTER_TO, ARRAY_OF, FUNCTION_RETURN
enum { POINTER_TO, ARRAY_OF, FUNCTION_RETURN };
/*********************************************
	@dec	used for syntax structure
			see funciton ParsePostfixDeclarator()
	@tyDrvList		used for semantics check ?
 *********************************************/
#define AST_DECLARATOR_COMMON   \
    AST_NODE_COMMON             \
    struct astDeclarator *dec;  \
    char *id;                   \
    TypeDerivList tyDrvList;
/***********************************
	typedef int INT32;
	tdname represent a typedef name.
	id:	name
	level:	the nesting level of the definition point.
		The nesting level of file scope is 0.
		The beginning of s compound statement will increment
		nesting level; the end of a compound statement will 
		decrement nesting level.
	overload:
		Indicate if the typedef name is used as variable instead of 
		a typedef name in nesting scope.
		
		typedef int a;
		int f(int a){
			// 'a' is a parameter instead of typedef name here.
		}
 ************************************/
typedef struct tdname
{
	char *id;
	int level;
	int overload;
	// see void PostCheckTypedef(void)
	int overloadLevel;
} *TDName;
/***********************************************
	Type Derivation
		ctor:	type constructor, can be array,function, pointer
		len:		array length
		qual:	pointer qualifier
		sig:		function signature
		next:	pointer to next type derivation list

	int * a[5];
		array of 5 pointer to int

		astPointerDeclarator --> astArrayDeclarator(5) -->  astDeclarator(a)

	The above syntax tree will be produced during syntax parsing.
	The declaration's binding order is similar as operator's priority.
	Identifier's priority is highest, belongs to direct declarator.
	Array and function's priority is same, belongs to array declarator and 
	function declarator respectively.
	Pointer's priority is lowest, belongs to pointer declarator.
		
	
	
	During semantic check ucc will construct the type derivation list 
	from bottom up.
	(pointer to) -->(array of 5) --> NIL

	function  DeriveType(...) will apply the type derivation in the type
	derivation list from left to right for the base type ty, 
	which in essence is in the order of type constructor's priority.
	For int * a[5], the result of type derivation is 
		"array of 5 pointer to int"
	From left to right:
		base type is int
		(1)  use "pointer to", we have "pointer to int"
		(2)  use "array of 5", we have "array of 5" "ponter to int",
			that is , "array of 5 pointer to int"
 ***********************************************/
// Each declarator (Function/Array/Pointer declarator) owns an attribue defined 
// below as struct typeDerivList object.
typedef struct typeDerivList
{
	int ctor;		// type constructor		pointer/function/array
	union
	{
		int len;	// array size
		int qual;	// pointer qualifier
		Signature sig;	// function signature
	};
	struct typeDerivList *next;
} *TypeDerivList;

typedef struct astSpecifiers *AstSpecifiers;

typedef struct astDeclarator
{
	AST_DECLARATOR_COMMON
} *AstDeclarator;
/*******************************************************
	init-declarator:
		declarator
		declarator = initializer
	initializer:
		assignment-expression
		{initializer-list}
		{initializer-list,}
	initializer-list:
		initializer
		initializer-list,initializer

	see	function  ParseInitializer(void)

	{{10,20,30},40,50}
	
	astInitializer		{{10,20,30},40,50}
		lbrace:1
		initials  ---> astInitializer 	{10,20,30}		
						lbrace:1
						initials		---->	astInitializer
												lbrace:0
												expr: 10
												next ----->	astInitializer
																	lbrace:0
																	expr:20
																	next ---->	astInitializer
																					lbrace:1
																					expr:30
																					next:NULL	
						next			----->	  astInitializer
													lbrace:0
													expr:		40
													next	---->	astInitializer
																		lbrace:0
																		expr:	50
																		next:	null
				
	
			
 *******************************************************/
typedef struct astInitializer
{
	AST_NODE_COMMON
	//  left brace  {			1/0		has or not
	int lbrace;
	union
	{
		AstNode initials;		// when lbrace is 1,	initializer-list
		AstExpression expr;		// when lbrace is 0, assignment-expression
	};
	/********************************************************
		When we do syntax parsing, we don't know the offset of each initialized
		field, So we use lbrace / initials / expr above.
		After semantics parsing, we know the offset of each initialized field,
		then we record these info in InitData for IR generation.
	// only used in semantic check 
	 ********************************************************/
	InitData idata;
} *AstInitializer;
/*********************************************
	init-declarator:
			declarator
			declarator = initializer
 *********************************************/
typedef struct astInitDeclarator
{
	AST_NODE_COMMON
	// declarator
	AstDeclarator dec;
	// initializer
	AstInitializer init;
} *AstInitDeclarator;
/***********************************************
	parameter-declaration:
		declaration-specifiers declarator
		declaration-specifiers abstract-declarator(opt)
 ************************************************/
typedef struct astParameterDeclaration
{
	AST_NODE_COMMON
	AstSpecifiers specs;
	AstDeclarator dec;
} *AstParameterDeclaration;
/**********************************************
	parameter-type-list:
		parameter-list
		parameter-list, ...
	parameter-list:
		parameter-declaration +	

	something like:
		int , int , int 
		int , int , ...
		in 
		f(int a, int b, int c), g(int a, int b, ...)

		a, b, c  is in Vector 'ids' in struct astFunctionDeclarator 
 **********************************************/
typedef struct astParameterTypeList
{
	AST_NODE_COMMON
	AstNode paramDecls;
	int ellipsis;
} *AstParameterTypeList;
/*************************************************
	direct-declarator:
		identifier
		(declarator)
		direct-declarator [constant-expression(opt)]	-----	Array
		direct-declarator (parameter-type-list)	-----	Function
		direct-declarator (identifier-list(opt))	-----	Function
 *************************************************/
typedef struct astFunctionDeclarator
{
	AST_DECLARATOR_COMMON
	Vector ids;
	AstParameterTypeList paramTyList;
	int partOfDef;
	Signature sig;
} *AstFunctionDeclarator;

typedef struct astArrayDeclarator
{
	AST_DECLARATOR_COMMON
	AstExpression expr;
} *AstArrayDeclarator;
// pointer:
// 			*	type-qualifier-list(opt) pointer
// type-qualifier-list:
//			type-qualifier
//			type-qualifier-list	type-qualifier
//	type-qualifier:
//			const	|	volatile
typedef struct astPointerDeclarator
{
	AST_DECLARATOR_COMMON
	AstNode tyQuals;
} *AstPointerDeclarator;
/*******************************************************
	struct-declarator:
		declarator
		declarator	opt	:	const-expression
	struct Data{
		int 	a;		---->   a
		int	b:3;	---->	b:3
	};
 *******************************************************/
typedef struct astStructDeclarator
{
	AST_NODE_COMMON
	AstDeclarator dec;
	AstExpression expr;
} *AstStructDeclarator;
/**********************************************
 struct-declaration:
	 specifier-qualifier-list	 struct-declarator-list
 specifier-qualifier-list:
	 (type-specifier | type-qualifier)+
 struct-declarator-list:
	 struct-declarator, struct-declarator, ....

	 (int | const| ...)+		a,b;

	see function ParseStructDeclaration(void),
		syntactically , we need specifier-qualifier-list here,
		excluding storage class,
		for simplicity, in UCC, we treat specifier-qualifier-list
		as declaration-specifier represented by astSpecifers Object.
 **********************************************/
typedef struct astStructDeclaration
{
	AST_NODE_COMMON
	AstSpecifiers specs;
	AstNode stDecs;
} *AstStructDeclaration;
/************************************************
	struct-or-union-specifier:
		struct-or-union		identifier(opt)	{struct-declaration-list}
		struct-or-union		identifier
 ************************************************/
typedef struct astStructSpecifier
{
	AST_NODE_COMMON
	char *id;
	// struct-declaration-list,
	// see function ParseStructOrUnionSpecifier()
	AstNode stDecls;
	int hasLbrace;
} *AstStructSpecifier;
//enumerator:		id
//					id = constant-expression
typedef struct astEnumerator
{
	AST_NODE_COMMON
	char *id;
	AstExpression expr;	
} *AstEnumerator;
// enum-specifier:		enum	id {enumerator-list}
//						enum	id
// enumerator-list:		enumerator +
typedef struct astEnumSpecifier
{
	AST_NODE_COMMON
	char *id;
	AstNode enumers;
} *AstEnumSpecifier;
// typedef int INT32	, 	INT32 is a TypedefName,
//	@id  	"INT32" 
typedef struct astTypedefName
{
	AST_NODE_COMMON
	char *id;
	Symbol sym;
} *AstTypedefName;
// even token is treated an abstract tree node ?
// see function ParseDeclarationSpecifiers(),
// storage classes: auto,static, extern,register, ..  are treated as astToken object.
//					typedef is a special case, syntactically considered a  storage class. 
typedef struct astToken
{
	AST_NODE_COMMON
	int token;
} *AstToken;
/*********************************************
	declaration-specifiers:
		storage-class-specifier	...		(static auto register...)
		type-specifier 		....			(int, void, ...)
		type-qualifier		...			(const volatile)
 *********************************************/
// (static | int | const| ...) +
struct astSpecifiers
{
	AST_NODE_COMMON
	AstNode stgClasses;
	AstNode tyQuals;
	AstNode tySpecs;
	// After semantics check ,we know the storage-class
	int sclass;
	Type ty;
};
/****************************************
	type-name:
		specifier-qualifier-list	abstract-declarator(opt)

			const int (*)() 	-----  is considered as a type name, anonymous
 ****************************************/
struct astTypeName
{
	AST_NODE_COMMON
	AstSpecifiers specs;
	AstDeclarator dec;
};
// declaration:	declaration-specifiers 	init-declarator-list
// init-declarator-list:		declarator
//							declarator = initializer
// declaration-specifiers:		(static|int|const...)+
// type-qualifier:		const	volatile
// storage-class-specifier:		static | auto | register | extern | typedef
// type-specifier:		int, void , char, struct-or-union-specifier, typedef-name
struct astDeclaration
{
	AST_NODE_COMMON
	// declaration-specifiers:	(staic | int | const | ...) +
	AstSpecifiers specs;
	// init-declarator-list:		id, id=300,
	AstNode initDecs;
};
/*********************************************************
	function-definition:
		declaration-specifiers(opt) declarator declaration-list(opt) compound-statement

		int	f(a,b,c) int a,b;double c; { ...}
		or
		int	f(int a,int b,double c)  { ...}
		or
	However:
		 function-declaration is considered as a special case of declaration,
		 with function-declarator.

		see function ParseExternalDeclaration();
 *********************************************************/
typedef struct astFunction
{
	AST_NODE_COMMON
	// declaration-specifiers(opt)
	AstSpecifiers specs;
	AstDeclarator dec;
	AstFunctionDeclarator fdec;
	//  declaration-list(opt)
	AstNode decls;
	// compound-statement
	AstStatement stmt;
	FunctionSymbol fsym;
	Label labels;
	Vector loops;
	Vector swtches;
	Vector breakable;
	int hasReturn;
} *AstFunction;

struct astTranslationUnit
{
	AST_NODE_COMMON
	/***********************************
		ExternalDeclarations
	 ************************************/
	AstNode extDecls;
};

void CheckLocalDeclaration(AstDeclaration decl, Vector v);
Type CheckTypeName(AstTypeName tname);

extern AstFunction CURRENTF;
#define	IsRecordSpecifier(spec)	(spec->kind == NK_StructSpecifier || spec->kind == NK_UnionSpecifier)


#endif

