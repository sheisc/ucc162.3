#include "ucl.h"
#include "grammer.h"
#include "ast.h"
#include "decl.h"


//  to allow	struct{	; },	empty struct declaration
static int FIRST_StructDeclaration[]   = { FIRST_DECLARATION, TK_SEMICOLON,0};
static int FF_StructDeclaration[]      = { FIRST_DECLARATION, TK_SEMICOLON,TK_RBRACE, 0};
//
static int FIRST_Function[]            = { FIRST_DECLARATION, TK_LBRACE, 0};
// to allow ";"
static int FIRST_ExternalDeclaration[] = { FIRST_DECLARATION, TK_MUL, TK_LPAREN, TK_SEMICOLON,0};




static Vector TypedefNames, OverloadNames;

int FIRST_Declaration[] = { FIRST_DECLARATION, 0};

static AstDeclarator ParseDeclarator(int abstract);
static AstSpecifiers ParseDeclarationSpecifiers(void);

/**
 * Get the identifier declared by the declarator dec
 */
//	int * a[3][5]		---->   OutermostID is "a"
static char* GetOutermostID(AstDeclarator dec)
{
	if (dec->kind == NK_NameDeclarator)
		return dec->id;

	return GetOutermostID(dec->dec);
}

const static char * nodeKindNames[] = {

		"NK_TranslationUnit", 	"NK_Function",		   "NK_Declaration",
		"NK_TypeName",			"NK_Specifiers",		  " NK_Token",				
		"NK_TypedefName", 		"NK_EnumSpecifier",	   "NK_Enumerator",			
		"NK_StructSpecifier", 	"NK_UnionSpecifier",	   "NK_StructDeclaration",	
		"NK_StructDeclarator",	"NK_PointerDeclarator",  "NK_ArrayDeclarator",		
		"NK_FunctionDeclarator",	"NK_ParameterTypeList",  "NK_ParameterDeclaration",
		"NK_NameDeclarator",		"NK_InitDeclarator",	   "NK_Initializer",
		
		"NK_Expression",
	
		"NK_ExpressionStatement", "NK_LabelStatement",	   "NK_CaseStatement",		
		"NK_DefaultStatement",	 "NK_IfStatement", 	   "NK_SwitchStatement",		
		"NK_WhileStatement",		"NK_DoStatement", 	   "NK_ForStatement", 	
		"NK_GotoStatement",		"NK_BreakStatement",	   "NK_ContinueStatement",		
		"NK_ReturnStatement", 	"NK_CompoundStatement"
};
// for trace
void PrintAstDeclarator(AstDeclarator dec){
	if(dec ){
		char * id = GetOutermostID(dec);
		if(id){
			PRINT_DEBUG_INFO(("%s", id));
		}
	}
	while (dec){		
		PRINT_DEBUG_INFO(("kind =  %d, %s",dec->kind,nodeKindNames[dec->kind]));
		dec = dec->dec;
	}

}


/**
 * UCC divides the syntax parsing and semantic check into two seperate passes.
 * But during syntax parsing, the parser needs to recognize C's typedef name.
 * In order to handle this, the parser performs minimum semantic check to 
 * process typedef name. For the parser, it is enough to know if an identifier 
 * is a typedef name.
 * 
 * The parser uses struct tdName to manage a typedef name. 
 * id: typedef name id
 * level: if two typedef names' id are same, the level will be the level of the 
 * typedef name who is in the outer scope. e.g.
 * typedef int a;
 * int f(void)
 * {
 *     typedef int a;
 * }
 * the a's level will be 0 instead of 1.
 * overload: A typedef name maybe used as variable in inner scope, e.g.
 * typedef int a;
 * int f(int a)
 * {
 * }
 * In f(), a is used as variable instead of typedef name.
 * For these cases, overload is set. When the scope terminates, overload 
 * is reset.
 *
 * The parser maintains two vectors: TypedefNames and OverloadNames
 * Whenever encountering a declaration, if it is a typedef name, records
 * it in the TypedefNames; else if the declaration redefines a typedef name
 * in outer scope as a variable, records it in the OverloadNames.
 * TypedefNames is for all the scope, while OverloadNames is for current scope,
 * when current scope terminates, reset the overload flags of all the item in
 * OverloadNames.
 */

/**
 * Decide if id is a typedef name
 */
static int IsTypedefName(char *id)
{
	Vector v = TypedefNames;
	TDName tn;
	/****************************************************
	 void f(void){
		 typedef struct{
			 int a[4];
		 }Data;
	 }
	 Data dt;		--------------  Even we are parsing here,
	 						'Data' is still in TypedefNames,
	 						But (tn->level) > Level.
	 						So we don't treat dt as a TypedefName.

	 						Data:  tn->level = 1, Level = 0
	 int main(){
		 return 0;
	 }

	 ****************************************************/
	FOR_EACH_ITEM(TDName, tn, v)
		if (tn->id == id && tn->level <= Level && ! tn->overload)
			return 1;
	ENDFOR

	return 0;
}

/**
 * If sclass is TK_TYPEDEF, records it in TypedefNames.
 * Otherwise, if id redefines a typedef name in outer scope,
 * records the typedef name in OverloadNames.
 */
//	This function only records TypedefName, not 'Check' as the function name indicates.
static void CheckTypedefName(int sclass, char *id)
{
	Vector v;
	TDName tn;
	/**********************************************************
		void f(int a,int){	----------- illegal in C, but we check it in declchk.c,
									not during syntax parsing.
		}
	 **********************************************************/
	if (id == NULL)
		return;

	v = TypedefNames;
	if (sclass == TK_TYPEDEF)
	{
		/************************************
			typedef	int	INT_32
			sclass:
					TK_TYPEDEF
			id:	
					INT_32
		 ************************************/
		FOR_EACH_ITEM(TDName, tn, v)
			if(tn->id == id)
			{
				/******************************************
				(1)
				typedef	const int CINT32;
				void f(int a,int b){
					typedef	const int CINT32;
				}
				
						 Level = 1, tn->level = 0 
				(2)
				void f(int a,int b){
					typedef	const int CINT32;
				}
				typedef	const int CINT32;
				
						Level = 0, tn->level = 1 
				 ******************************************/
				//PRINT_DEBUG_INFO(("Level = %d, tn->level = %d ", Level, tn->level));
				if (Level < tn->level)
					tn->level = Level;
				return;
			}
		ENDFOR
		/*************************************
			If there isn't a typedefed name , e.g.  "INT_32",
			we insert it into TypedefNames
		 *************************************/
		ALLOC(tn);
		tn->id = id;
		tn->level = Level;
		tn->overload = 0;
		INSERT_ITEM(v, tn);
	}
	else
	{
		/*************************************
			typedef	int	a;
			int f(int a){
				// 'a' is a parameter instead of typedef name in f()'s definition
			}
		 *************************************/
		FOR_EACH_ITEM(TDName, tn, v)
			if (tn->id == id && Level > tn->level)
			{
				tn->overload = 1;
				tn->overloadLevel = Level;
				INSERT_ITEM(OverloadNames, tn);
				return;
			}
		ENDFOR
	}
}

/**
 * Perform minimum semantic check for each declaration 
 */
//	Anytime, when we have parsed a declaration,
//	int * a[3][5], b;	-----> 	'a' is a declarator id, b is also a declarator id.
//	we call	PreCheckTypedef(...) for each declaration id in the declaration.
// The real work is done by CheckTypedefName(...)
static void PreCheckTypedef(AstDeclaration decl)
{
	AstNode p;
	int sclass = 0;
	// If 'typedef' is in the linked list pointed by stgClasses,
	// it should be the first one .   
	// Only one storage-class-specifier is allowed ?
	//	see function CheckDeclarationSpecifiers();
	if (decl->specs->stgClasses != NULL)
	{
		sclass = ((AstToken)decl->specs->stgClasses)->token;
	}

	p = decl->initDecs;
	/*****************************************
		(1) typedef const int CINT_32;		
			is considered as a declaration syntactically.
			
		"typedef":
				storage-class-specifier
		"int":
				type-specifier
		"const":
				type-qualifier
		"CINT_32":
				declarator.

			int this case, the outer most id "CINT_32"  is a typedefed name
		(2) const int CINT_32;
			int this case, the outer most id "CINT_32" is a normal declarator id.
	 *****************************************/
	while (p != NULL)
	{
		CheckTypedefName(sclass, GetOutermostID(((AstInitDeclarator)p)->dec));
		p = p->next;
	}
}

static void PrintOverloadNames(void){
	TDName tn;
	PRINT_DEBUG_INFO(("PrintOverloadNames(void): "));
	FOR_EACH_ITEM(TDName, tn, OverloadNames)
		PRINT_DEBUG_INFO(("Level %d : tn: id = %s, level = %d,overload = %d, overloadLevel = %d",
								Level,tn->id,tn->level,tn->overload,tn->overloadLevel));	
	ENDFOR	
}

/**
 * When current scope except file scope terminates, clear OverloadNames.
 */
void PostCheckTypedef(void)
{

	TDName tn;
	int overloadCount = 0;
	// PrintOverloadNames();
	
	FOR_EACH_ITEM(TDName, tn, OverloadNames)
		if(Level <= (tn->overloadLevel)){
			tn->overload = 0;
		}else if(tn->overload  != 0){
			overloadCount++;
		}
	ENDFOR
		
	// PrintOverloadNames();		
	if(overloadCount == 0){
		OverloadNames->len = 0;
	}

}

/**
 *  initializer:
 *		assignment-expression
 *		{ initializer-list }
 *		{ initializer-list , }
 *
 *  initializer-list:
 *		initializer
 *		initializer-list, initializer
 */
// see the comments near "struct astInitializer" in header file for detail. 
static AstInitializer ParseInitializer(void)
{
	AstInitializer init;
	AstNode *tail;

	CREATE_AST_NODE(init, Initializer);
	// { {1,2,3},4,5,6}
	if (CurrentToken == TK_LBRACE)
	{
		init->lbrace = 1;
		NEXT_TOKEN;
		init->initials = (AstNode)ParseInitializer();
		tail = &init->initials->next;
		while (CurrentToken == TK_COMMA)
		{
			NEXT_TOKEN;
			if (CurrentToken == TK_RBRACE)
				break;
			*tail = (AstNode)ParseInitializer();
			tail = &(*tail)->next;
		}
		Expect(TK_RBRACE);
	}
	else
	{
		init->lbrace = 0;
		init->expr = ParseAssignmentExpression();
	}

	return init;
}

/**
 *  init-declarator:
 *		declarator
 *      declarator = initializer
 */
static AstInitDeclarator ParseInitDeclarator(void)
{
	AstInitDeclarator initDec;

	CREATE_AST_NODE(initDec, InitDeclarator);

	initDec->dec = ParseDeclarator(DEC_CONCRETE);
	if (CurrentToken == TK_ASSIGN)
	{
		NEXT_TOKEN;
		initDec->init = ParseInitializer();
	}

	return initDec;
}

/**
 *  direct-declarator:
 *		ID
 *		( declarator )
 *
 *  direct-abstract-declarator:
 *		( abstract-declarator)
 *		nil
 */
static AstDeclarator ParseDirectDeclarator(int kind)
{
	AstDeclarator dec;

	// see examples/cast/funcCast.c
	if (CurrentToken == TK_LPAREN)
	{
		int t;
		
		BeginPeekToken();
		t = GetNextToken();
		if (t != TK_ID && t !=TK_LPAREN && t != TK_MUL){
			// we are sure that it cant' be the prefix of a  Declarator
			EndPeekToken();	
			CREATE_AST_NODE(dec, NameDeclarator);
		}else{
			EndPeekToken();
			NEXT_TOKEN;
			dec = ParseDeclarator(kind);
			Expect(TK_RPAREN);
		}
		return dec;
	}

	CREATE_AST_NODE(dec, NameDeclarator);

	if (CurrentToken == TK_ID)
	{
		if (kind == DEC_ABSTRACT)
		{
			Error(&TokenCoord, "Identifier is not permitted in the abstract declarator");
		}		
		dec->id = TokenValue.p;
		// PRINT_DEBUG_INFO(("dec->id :%s ",dec->id));
		NEXT_TOKEN;
	}
	else if (kind == DEC_CONCRETE)
	{
		Error(&TokenCoord, "Expect identifier");
	}

	return dec;
}

/**
 *  parameter-declaration:
 *		declaration-specifiers declarator
 *		declaration-specifiers abstract-declarator
 */
//  one declaration, such as "int a "  in 	"f(int a, int b, int c)"
static AstParameterDeclaration ParseParameterDeclaration(void)
{
	AstParameterDeclaration paramDecl;

	CREATE_AST_NODE(paramDecl, ParameterDeclaration);

	paramDecl->specs = ParseDeclarationSpecifiers();
	// f(int a, int b, int c);	--->  CONCRETE
	// or
	// f(int, int , int)		--->	ABSTRACT
	/*************************************************************
		Here is Declarator in f(int a,int b, int c), not InitDeclarator.
		Because in C,
			f(int a = 3, int b = 5, int c = 6) is illegal.
	 *************************************************************/
	paramDecl->dec   = ParseDeclarator(DEC_ABSTRACT | DEC_CONCRETE);

	return paramDecl;
}

/**
 *  parameter-type-list:
 *		parameter-list
 *		parameter-list , ...
 *
 *  parameter-list:
 *		parameter-declaration
 *		parameter-list , parameter-declaration
 *
 *  parameter-declaration:
 *		declaration-specifiers declarator
 *		declaration-specifiers abstract-declarator
 */
 /**************************************************

		"int a, int b, int c "  in 	f(int a , int b , int c )
		or
		"int,int,int" in g(int,int,int)
  ***************************************************/
AstParameterTypeList ParseParameterTypeList(void)
{
	AstParameterTypeList paramTyList;
	AstNode *tail;

	CREATE_AST_NODE(paramTyList, ParameterTypeList);

	paramTyList->paramDecls = (AstNode)ParseParameterDeclaration();
	tail = &paramTyList->paramDecls->next;
	while (CurrentToken == TK_COMMA)
	{
		NEXT_TOKEN;
		if (CurrentToken == TK_ELLIPSIS)
		{
			paramTyList->ellipsis = 1;
			NEXT_TOKEN;
			break;
		}
		*tail = (AstNode)ParseParameterDeclaration();
		tail = &(*tail)->next;
	}

	return paramTyList;
}

/**
 *  postfix-declarator:
 *		direct-declarator
 *		postfix-declarator [ [constant-expression] ]
 *		postfix-declarator ( parameter-type-list)
 *		postfix-declarator ( [identifier-list] )
 *
 *  postfix-abstract-declarator:
 *		direct-abstract-declarator
 *		postfix-abstract-declarator ( [parameter-type-list] )
 *		postfix-abstract-declarator [ [constant-expression] ]
 */
/***************************************************
	int arr[3];	
	int (arr)[3];
	int (*)[3] arr;		----	error		

	int typedef (* ARR)[3] ;		---> right from syntactic view, but not normal case
	typedef int (* ARR)[3] ;
			typedef a new type, name it ARR.
	int (*arr)[3];		arr is a pointer variable, which points an array of 3 ints

	
	direct-declarator:
			arr
 ***************************************************/
static AstDeclarator ParsePostfixDeclarator(int kind)
{
	AstDeclarator dec = ParseDirectDeclarator(kind);

	while (1)
	{
		if (CurrentToken == TK_LBRACKET)
		{
			AstArrayDeclarator arrDec;

			CREATE_AST_NODE(arrDec, ArrayDeclarator);
			arrDec->dec = dec;

			NEXT_TOKEN;
			if (CurrentToken != TK_RBRACKET)
			{
				arrDec->expr = ParseConstantExpression();
			}
			Expect(TK_RBRACKET);

			dec = (AstDeclarator)arrDec;
		}
		else if (CurrentToken == TK_LPAREN)
		{
			/// notice: for abstract declarator, identifier list is not permitted,
			/// but we leave this to semantic check
			AstFunctionDeclarator funcDec;

			CREATE_AST_NODE(funcDec, FunctionDeclarator);
			funcDec->dec = dec;
			
			NEXT_TOKEN;
			if (IsTypeName(CurrentToken))
			{
				/*********************************************
					f(int a, int b, int c);
				 *********************************************/
				funcDec->paramTyList = ParseParameterTypeList();
			}
			else
			{
				funcDec->ids = CreateVector(4);
				if (CurrentToken == TK_ID)	
				{
					/********************************************
						f(a,b,c);
							a	,b	,c
					 ********************************************/
					INSERT_ITEM(funcDec->ids, TokenValue.p);

					NEXT_TOKEN;
					while (CurrentToken == TK_COMMA)
					{
						NEXT_TOKEN;
						if (CurrentToken == TK_ID)
						{
							INSERT_ITEM(funcDec->ids, TokenValue.p);
						}
						Expect(TK_ID);
					}
				}
			}
			Expect(TK_RPAREN);
			dec = (AstDeclarator)funcDec;
		}
		else
		{
			return dec;
		}
	}
}

/**
 *  abstract-declarator:
 *		pointer
 *		[pointer] direct-abstract-declarator
 *
 *  direct-abstract-declarator:
 *		( abstract-declarator )
 *		[direct-abstract-declarator] [ [constant-expression] ]
 *		[direct-abstract-declarator] ( [parameter-type-list] )
 *
 *  declarator:
 *		pointer declarator
 *		direct-declarator
 *
 *  direct-declarator:
 *		ID
 *		( declarator )
 *		direct-declarator [ [constant-expression] ]
 *		direct-declarator ( parameter-type-list )
 *		direct-declarator ( [identifier-list] )
 *
 *  pointer:
 *		* [type-qualifier-list]
 *		* [type-qualifier-list] pointer
 *
 *  We change the above standard grammar into more suitable form defined below.
 *  abstract-declarator:
 *		* [type-qualifer-list] abstract-declarator
 *		postfix-abstract-declarator
 *	
 *  postfix-abstract-declarator:
 *		direct-abstract-declarator
 *		postfix-abstract-declarator [ [constant-expression] ]
 *		postfix-abstrace-declarator( [parameter-type-list] )
 *		
 *  direct-abstract-declarator:
 *		( abstract-declarator )
 *		NULL
 *
 *  declarator:
 *		* [type-qualifier-list] declarator
 *		postfix-declarator
 *
 *  postfix-declarator:
 *		direct-declarator
 *		postfix-declarator [ [constant-expression] ]
 *		postfix-declarator ( parameter-type-list)
 *		postfix-declarator ( [identifier-list] )
 *
 *  direct-declarator:
 *		ID
 *		( declarator )
 *
 *	The declartor is similar as the abstract declarator, we use one function
 *	ParseDeclarator() to parse both of them. kind indicate to parse which kind
 *	of declarator. The possible value can be:
 *	DEC_CONCRETE: parse a declarator
 *	DEC_ABSTRACT: parse an abstract declarator
 *	DEC_CONCRETE | DEC_ABSTRACT: both of them are ok
 */
/*********************************************************
	For function declaration:
		DEC_CONCRETE|DEC_ABSTRACT
	For function definition
		DEC_CONCRETE
	For type-name, 	sizeof(type-name),	 (type-name)(expr)
		DEC_ABSTRACT
 *********************************************************/
static AstDeclarator ParseDeclarator(int kind)
{
	if (CurrentToken == TK_MUL)
	{
		// * [type-qualifier-list] declarator
		// *  is TK_MUL
		AstPointerDeclarator ptrDec;
		AstToken tok;
		AstNode *tail;

		CREATE_AST_NODE(ptrDec, PointerDeclarator);
		tail = &ptrDec->tyQuals;

		NEXT_TOKEN;
		//	[type-qualifier-list]
		while (CurrentToken == TK_CONST || CurrentToken == TK_VOLATILE)
		{
			CREATE_AST_NODE(tok, Token);
			tok->token = CurrentToken;
			*tail = (AstNode)tok;
			tail = &tok->next;
			NEXT_TOKEN;
		}
		// declarator
		ptrDec->dec = ParseDeclarator(kind);

		return (AstDeclarator)ptrDec;
	}

	return ParsePostfixDeclarator(kind);
}

/**
 *  struct-declarator:
 *		declarator
 *		[declarator] : constant-expression
 */
/**************************************************
	struct Data{
		int	:4;	//---------------------- legal
		int a:12;
		int b:4;
		int c;
	};

	
	a:12		----> 	struct declarator
	b:4			---->	struct declarator
	c			---->	struct declarator
	
 **************************************************/
static AstStructDeclarator ParseStructDeclarator(void)
{
	AstStructDeclarator stDec;
	//PRINT_DEBUG_INFO(("ParseStructDeclarator(void)"));
	CREATE_AST_NODE(stDec, StructDeclarator);
	// [declarator]
	if (CurrentToken != TK_COLON)
	{
		stDec->dec = ParseDeclarator(DEC_CONCRETE);
	}
	// : constant-expression
	if (CurrentToken == TK_COLON)
	{
		NEXT_TOKEN;
		stDec->expr = ParseConstantExpression();
	}

	return stDec;
}

/**
 *  struct-declaration:
 *		specifier-qualifer-list struct-declarator-list ;
 *
 *  specifier-qualifier-list:
 *		type-specifier [specifier-qualifier-list]
 *		type-qualifier [specifier-qualifier-list]
 *
 *  struct-declarator-list:
 *		struct-declarator
 *		struct-declarator-list , struct-declarator
 */
 /*******************************************************

 	struct Data{
 		int a;	------>  one struct declaration
 		int b;	------>  another struct declaration
 		int c;	......
 	};
  ********************************************************/
static AstStructDeclaration ParseStructDeclaration(void)
{
	AstStructDeclaration stDecl;
	AstNode *tail;
	//PRINT_DEBUG_INFO(("ParseStructDeclaration(void)"));
	CREATE_AST_NODE(stDecl, StructDeclaration);

	// to support 	struct {	;	},	empty struct/union declaration
	if (CurrentToken == TK_SEMICOLON){		
		NEXT_TOKEN;
		return NULL;
	}
	/// declaration specifiers is a superset of speicifier qualifier list,
	/// for simplicity, just use ParseDeclarationSpecifiers() and check if
	/// there is storage class

	// specifier-qualifer-list
	stDecl->specs = ParseDeclarationSpecifiers();
	if (stDecl->specs->stgClasses != NULL)
	{
		Error(&stDecl->coord, "Struct/union member should not have storage class");
		stDecl->specs->stgClasses = NULL;
	}
	if (stDecl->specs->tyQuals == NULL && stDecl->specs->tySpecs == NULL)
	{
		Error(&stDecl->coord, "Expect type specifier or qualifier");
	}

	// an extension to C89, supports anonymous struct/union member in struct/union
	if (CurrentToken == TK_SEMICOLON)
	{	
		NEXT_TOKEN;
		return stDecl;		
	}
	// struct-declarator-list
	//				struct-declarator,struct-declarator, ...., struct-declarator
	stDecl->stDecs = (AstNode)ParseStructDeclarator();
	tail = &stDecl->stDecs->next;
	while (CurrentToken == TK_COMMA)
	{
		NEXT_TOKEN;
		*tail = (AstNode)ParseStructDeclarator();
		tail = &(*tail)->next;
	}
	Expect(TK_SEMICOLON);

	return stDecl;
}

/**
 *  struct-or-union-specifier:
 *		struct-or-union [identifier] { struct-declaration-list }
 *		struct-or-union identifier
 *
 *  struct-or-union:
 *		struct
 *		union
 *
 *  struct-declaration-list:
 *      struct-declaration
 *		struct-declaration-list struct-declaration
 */
static AstStructSpecifier ParseStructOrUnionSpecifier(void)
{
	AstStructSpecifier stSpec;
	AstNode *tail;
	// PRINT_DEBUG_INFO((" ParseStructOrUnionSpecifier(void)"));
	CREATE_AST_NODE(stSpec, StructSpecifier);
	if (CurrentToken == TK_UNION)
	{
		stSpec->kind = NK_UnionSpecifier;
	}

	NEXT_TOKEN;
	switch (CurrentToken)
	{
	case TK_ID:
		/***************************************
			struct  ABC;
			or
			union  ABC;
		 ***************************************/
		stSpec->id = TokenValue.p;
		NEXT_TOKEN;
		if (CurrentToken == TK_LBRACE)
			goto lbrace;

		return stSpec;

	case TK_LBRACE:
lbrace:
		/******************************************
			struct ABC
			{	------------ we are here
			}
			struct 
			{	------------ we are here
			}
		 ******************************************/
		NEXT_TOKEN;
		stSpec->hasLbrace = 1;

		if (CurrentToken == TK_RBRACE)
		{
			// see examples/struct/empty.c		
			NEXT_TOKEN;
			return stSpec;
		}

		// FIXME: use a better way to handle errors in struct declaration list
		tail = &stSpec->stDecls;
		while (CurrentTokenIn(FIRST_StructDeclaration))
		{
			// the return value could be NULL now.
			*tail = (AstNode)ParseStructDeclaration();
			if(*tail != NULL){
				tail = &(*tail)->next;
			}
			SkipTo(FF_StructDeclaration, "the start of struct declaration or }");
		}
		Expect(TK_RBRACE);
		return stSpec;

	default:
		Error(&TokenCoord, "Expect identifier or { after struct/union");
		return stSpec;
	}
}

/**
 *  enumerator:
 *		enumeration-constant
 *		enumeration-constant = constant-expression
 *
 *  enumeration-constant:
 *		identifier
 */
/***************************************
	enum COLOR{RED, GREEN = 5, BLUE};

	RED is an enumerator
 ****************************************/
static AstEnumerator ParseEnumerator(void)
{
	AstEnumerator enumer;

	CREATE_AST_NODE(enumer, Enumerator);

	if (CurrentToken != TK_ID)
	{
		Error(&TokenCoord, "The eumeration constant must be identifier");
		return enumer;
	}

	enumer->id = TokenValue.p;
	NEXT_TOKEN;
	//  = constant-expression
	if (CurrentToken == TK_ASSIGN)
	{
		NEXT_TOKEN;
		enumer->expr = ParseConstantExpression();
	}

	return enumer;
}

/**
*  enum-specifier
*		enum [identifier] { enumerator-list }
*		enum [identifier] { enumerator-list , }
*		enum identifier
*
*  enumerator-list:
*		enumerator
*		enumerator-list , enumerator
*/
/***************************************
		enum COLOR{RED, GREEN = 5, BLUE};
			or
		enum COLOR

		are all enum-specifiers


		"RED, GREEN = 5, BLUE"
				is enumerator-list.
 *****************************************/
static AstEnumSpecifier ParseEnumSpecifier(void)
{
	AstEnumSpecifier enumSpec;
	AstNode *tail;

	CREATE_AST_NODE(enumSpec, EnumSpecifier);

	NEXT_TOKEN;
	if (CurrentToken == TK_ID)
	{
		enumSpec->id = TokenValue.p;
		NEXT_TOKEN;
		if (CurrentToken == TK_LBRACE)
			goto enumerator_list;
	}
	else if (CurrentToken == TK_LBRACE)
	{
enumerator_list:
		//	enumerator-list:	"RED, GREEN = 5, BLUE  }"
		//						"RED, GREEN = 5, BLUE , }"
		NEXT_TOKEN;		
		/******************************************
			enum Color{

			}a;
			expected identifier before '}' token
		 ******************************************/
		if(CurrentToken == TK_RBRACE){
			NEXT_TOKEN;
			Error(&TokenCoord, "Expect identifier before '}' token");
			return enumSpec;
		}
		enumSpec->enumers = (AstNode)ParseEnumerator();
		tail = &enumSpec->enumers->next;
		while (CurrentToken == TK_COMMA)
		{
			NEXT_TOKEN;
			if (CurrentToken == TK_RBRACE)
				break;
			*tail = (AstNode)ParseEnumerator();
			tail = &(*tail)->next;
		}
		Expect(TK_RBRACE);
	}
	else
	{
		Error(&TokenCoord, "Expect identifier or { after enum");
	}

	return enumSpec;
}

/**
 *  declaration-specifiers:
 *		storage-class-specifier [declaration-specifiers]
 *		type-specifier [declaration-specifiers]
 *		type-qualifier [declaration-specifiers]
 *
 *  storage-class-specifier:
 *		auto
 *		register
 *		static
 *		extern
 *		typedef
 *
 *  type-qualifier:
 *		const
 *		volatile
 *
 *  type-specifier:
 *		void
 *		char
 *		short
 *		int
 *		long
 *		float
 *		double
 *		signed
 *		unsigned
 *		struct-or-union-specifier
 *		enum-specifier
 *		typedef-name
 */
static AstSpecifiers ParseDeclarationSpecifiers(void)
{
	AstSpecifiers specs;
	AstToken tok;
	AstNode *scTail, *tqTail, *tsTail;
	int seeTy = 0;

	CREATE_AST_NODE(specs, Specifiers);
	// static , auto, register, ...
	scTail = &specs->stgClasses;
	// const , volatile
	tqTail = &specs->tyQuals;
	// int, double ,	...
	tsTail = &specs->tySpecs;
	/****************************************************
		The real parsing we do here is :
			declaration-specifiers(opt)
		for example:
			f(void){	--------- when we see f, we return immediately.
			}
		So we must check whether it is legal to miss the declaration-specifiers 
		at later stage.
	 ****************************************************/
next_specifier:
	switch (CurrentToken)
	{
	case TK_AUTO:
	case TK_REGISTER:
	case TK_STATIC:
	case TK_EXTERN:
	case TK_TYPEDEF:
		// storage-class-specifier
		CREATE_AST_NODE(tok, Token);
		tok->token = CurrentToken;
		*scTail = (AstNode)tok;
		scTail = &tok->next;
		NEXT_TOKEN;
		break;

	case TK_CONST:
	case TK_VOLATILE:
		// type-qualifier
		CREATE_AST_NODE(tok, Token);
		tok->token = CurrentToken;
		*tqTail = (AstNode)tok;
		tqTail = &tok->next;
		NEXT_TOKEN;
		break;

	case TK_VOID:
	case TK_CHAR:
	case TK_SHORT:
	case TK_INT:
	case TK_INT64:
	case TK_LONG:
	case TK_FLOAT:
	case TK_DOUBLE:
	case TK_SIGNED:
	case TK_UNSIGNED:
		/******************************************
			typedef signed int t;
			typedef int plain;
			struct tag {
				const t;	//	const signed int;	declare nothing.
				plain r;	//	int r;
				unsigned t;	// unsigned int t;
			};
		 ******************************************/
		// These basic types are type-specifier
		CREATE_AST_NODE(tok, Token);
		tok->token = CurrentToken;
		*tsTail = (AstNode)tok;
		tsTail = &tok->next;
		seeTy = 1;
		NEXT_TOKEN;
		break;

	case TK_ID:
		//	when we meet an id, we should find out whether is a typedefed name.
		//  A typedefed name is also treated as type-specifier
		if (! seeTy && IsTypedefName(TokenValue.p))
		{
			AstTypedefName tname;

			CREATE_AST_NODE(tname, TypedefName);

			tname->id = TokenValue.p;
			*tsTail = (AstNode)tname;
			tsTail = &tname->next;
			NEXT_TOKEN;
			seeTy = 1;
			break;
		} 
		return specs;

	case TK_STRUCT:
	case TK_UNION:
		// struct-or-union-specifier is also considered as type-specifier
		*tsTail = (AstNode)ParseStructOrUnionSpecifier();
		tsTail = &(*tsTail)->next;
		seeTy = 1;
		break;

	case TK_ENUM:
		// enum-specifier is considered as type-specifier too.
		*tsTail = (AstNode)ParseEnumSpecifier();
		tsTail = &(*tsTail)->next;
		seeTy = 1;
		break;

	default:
		return specs;
	}
	goto next_specifier;
}

int IsTypeName(int tok)
{
	return tok == TK_ID ? IsTypedefName(TokenValue.p) : (tok >= TK_AUTO && tok <= TK_VOID);
}

/**
 * type-name:
 *     specifier-qualifier-list abstract-declarator
 */
//(1) type-name is mainly used for cast-expresion and sizeof(type-name), but not for declaration.
//(2) storage-class-specifier is excluded here, 
AstTypeName ParseTypeName(void)
{
	AstTypeName tyName;

	CREATE_AST_NODE(tyName, TypeName);

	tyName->specs = ParseDeclarationSpecifiers();
	//	when declaring a variable, we allocate memory for it.
	// so we need storage-class-specifier in a declaration to specify where the variable is .
	// However, when it comes to only type information, we just care for the type itself.
	if (tyName->specs->stgClasses != NULL)
	{
		Error(&tyName->coord, "type name should not have storage class");
		tyName->specs->stgClasses = NULL;
	}
	tyName->dec = ParseDeclarator(DEC_ABSTRACT);

	return tyName;
}

/**
 *  The function defintion and declaration have some common parts:
 *	declaration-specifiers [init-declarator-list]
 *  if we found that the parts followed by a semicolon, then it is a declaration
 		see ParseDeclaration().
 *  or if the init-declarator list is a stand-alone declarator, then it may be
 *  a function definition.
 */
 // "int;" can be seen as  a declaration ? 
 // "int a;"
 //	parse :		declaration-specifiers [init-declarator-list]
 // In fact ,it is not perfectly suitable for function-definition,
 // So we need GetFunctionDeclarator() later.
static AstDeclaration ParseCommonHeader(void)
{
	AstDeclaration decl;
	AstNode *tail;

	CREATE_AST_NODE(decl, Declaration);
	// declaration-specifiers
	decl->specs = ParseDeclarationSpecifiers();
	if (CurrentToken != TK_SEMICOLON)
	{
		//a = 3, b , c=50;
		// or
		// f(int a, int b);	
		// [init-declarator-list]
		decl->initDecs = (AstNode)ParseInitDeclarator();
		tail = &decl->initDecs->next;
		while (CurrentToken == TK_COMMA)
		{
			NEXT_TOKEN;
			*tail = (AstNode)ParseInitDeclarator();
			tail = &(*tail)->next;
		}
	}

	return decl;
}
/******************************************
declaration
	declaration-specifiers [init-declarator-list]
 ******************************************/
AstDeclaration ParseDeclaration(void)
{
	AstDeclaration decl;

	decl = ParseCommonHeader();
	Expect(TK_SEMICOLON);
	PreCheckTypedef(decl);

	return decl;
}

/**
 * If initDec is a legal function declarator, return it 
 */
// Because of the inaccurate processing of ParseCommonHeader(void)  ,
// So we need GetFunctionDeclarator(...) to check whether it's a function-declarator.
static AstFunctionDeclarator GetFunctionDeclarator(AstInitDeclarator initDec)
{
	AstDeclarator dec;
	// (1) initDec->next == NULL
	//		for function-definition, only one astInitDeclarator in the node-list.
	//		int abc, g(int a,int b) { } 	is illegal.
	// (2) initDec->init == NULL
	//		for function-definition,
	//		int g(int a, int b) = 30;		is illegal.
	// That is , function declarator has not sibling node, without  "= {1,2,3}"  either.
	// So , for function declarator:
	// initDec->next is NULL, and  initDec->init is NULL.
	if (initDec == NULL || initDec->next != NULL || initDec->init != NULL)
		return NULL;

	dec = initDec->dec;
	/****************************************************************
	see	examples/declaration/complicateDecl.c
		int (* f(void))(void){
			return 0;
		}
		NK_FunctionDeclartor->NK_PointerDeclarator-> NK_FunctionDeclarator->NK_NameDeclarator
	 ****************************************************************/
	while (dec ){
		if(dec->kind == NK_FunctionDeclarator 
			&& dec->dec && dec->dec->kind == NK_NameDeclarator){
			break;
		}
		dec = dec->dec;
	}
	return (AstFunctionDeclarator)dec;
}

/**
 *  external-declaration:
 *		function-definition
 *		declaration
 *
 *  function-definition:
 *		declaration-specifiers declarator [declaration-list] compound-statement
 *
 *  declaration:
 *		declaration-specifiers [init-declarator-list] ;
 *
 *  declaration-list:
 *		declaration
 *		declaration-list declaration
 */
static AstNode ParseExternalDeclaration(void)
{
	AstDeclaration decl = NULL;
	AstInitDeclarator initDec = NULL;
	AstFunctionDeclarator fdec;
	/*********************************************************
		function-definition:
			declaration-specifiers(opt)	declarator  ....
		declaration:
			delcaration-specifiers(opt)	init-declarator-list(opt)

		ParseCommomHeader parses :
			delcaration-specifiers(opt)	init-declarator-list(opt)

		that is, the prefix of function-definition is treated 
			as declaration.  Of course, this is not accurate, 
			so we need GetFunctionDeclarator(...) to further check whether
			it is a real function declarator.
		The key point here is to avoid looking ahead to distinguish whether
		we are parsing function-definition or external-declaration.
	 *********************************************************/
	decl = ParseCommonHeader();
	initDec = (AstInitDeclarator)decl->initDecs;
	/***********************************************************
		see ParseDeclarationSpecifiers(),
			(1) storage class information is stored in astToken object.
			(2) typedef is considered a storage-class-specifier syntactically as static, register, extern ,auto.
			(3) only one storage-class-specifier is allowed
	 ***********************************************************/
	if (decl->specs->stgClasses != NULL && ((AstToken)decl->specs->stgClasses)->token == TK_TYPEDEF)
		goto not_func;
	// further check whether it's a real function declarator ?
	fdec = GetFunctionDeclarator(initDec);
	if (fdec != NULL)
	{
		AstFunction func;
		AstNode *tail;
		
		if (CurrentToken == TK_SEMICOLON)
		{
			NEXT_TOKEN;
			/****************************************************************
				If current token is not left brace here, we have encounted just a function 
				declaration, not a definition 
			 *****************************************************************/
			if (CurrentToken != TK_LBRACE)
				return (AstNode)decl;

			// maybe a common error, function definition follows ;
			Error(&decl->coord, "maybe you accidently add the ;");
		}
		else if (fdec->paramTyList && CurrentToken != TK_LBRACE)
		{			
			// a common error, function declaration loses ;
			//	;	----> semicolon
			goto not_func;
		}
		/***************************************
			int f(int a, int a){	----->  we have parsed the left part, that is , function declaration.
											when CurrentToken is TK_LBRACE
											and fdec->paramTyList != NULL
					....
			}
			or
			int k(){			------>  when fdec->paramTyList is NULL.
			}
			or
			int f(a,b,c) 			------>  we are here.
											when fdec->paramTyList is NULL.
			int a,b;double c{
			}
		 ***************************************/
		CREATE_AST_NODE(func, Function);
		// the function declaration's coord and specs are treated as the whole function's.
		func->coord = decl->coord;
		func->specs = decl->specs;
		/**********************************
		int ** f(int a, int b);
		 astPointerDeclarator-> astPointerDeclarator ->  astFunctionDeclarator --> astDeclarator
			(dec)									(fdec )
		 ***********************************/
		func->dec = initDec->dec;
		func->fdec = fdec;
		
		Level++;
		if (func->fdec->paramTyList)
		{
			/**************************************
				int f(int a, int a){ -----------> we are here
			 ***************************************/
			AstNode p = func->fdec->paramTyList->paramDecls;

			while (p)
			{
				/***************************************
				typedef int a;
				void f(int a){
					// we should record 'a' is overloaded in the function definition.
				}
				 ***************************************/
				//  TK_BEGIN is 0,   TK_AUTO is 1, see lex.h
				CheckTypedefName(0, GetOutermostID(((AstParameterDeclaration)p)->dec));
				p = p->next;
			}
		}
		tail = &func->decls;
		/*********************************************
			int f(a,b,c) 			------>  we are here.
											when fdec->paramTyList is NULL.
			int a,b;double c;{
			}
		 *********************************************/
		while (CurrentTokenIn(FIRST_Declaration))
		{
			*tail = (AstNode)ParseDeclaration();
			tail = &(*tail)->next;
		}
		Level--;

		func->stmt = ParseCompoundStatement();

		return (AstNode)func;
	}

not_func:
	/*******************************************************
		a = 3.0;		---------- see ParseDeclarationSpecifiers()
		void f(void){
			//b = 3;	----------- error
		}
	 *******************************************************/
	if(!decl->specs->stgClasses && !decl->specs->tyQuals && !decl->specs->tySpecs){
		Warning(&decl->coord,"declaration specifier missing, defaulting to 'int'");
		// see void CheckDeclarationSpecifiers(decl->specs);for defaulting to 'int'
	}
	/**********************************
		If we get here,
			we should do the same thing as 
			function ParseDeclaration().
	 **********************************/
	Expect(TK_SEMICOLON);
	PreCheckTypedef(decl);

	return (AstNode)decl;
}

/**
 *  translation-unit:
 *		external-declaration
 *		translation-unit external-declaration
 */
AstTranslationUnit ParseTranslationUnit(char *filename)
{
	/*************************************
		TranslationUnit AST Node consists of 
			list of AstDeclaration nodes
	 *************************************/
	AstTranslationUnit transUnit;
	AstNode *tail;

	ReadSourceFile(filename);

	TokenCoord.filename = filename;
	TokenCoord.line = TokenCoord.col = TokenCoord.ppline = 1;
	TypedefNames = CreateVector(8);
	OverloadNames = CreateVector(8);
	// allocate a AST_NODE  and set its kind to NK_TranslationUnit.
	CREATE_AST_NODE(transUnit, TranslationUnit);
	tail = &transUnit->extDecls;
	/**********************************************
		The classic scheme of Top-Down recursive parsing is :
			next_token();
			parse();
	 **********************************************/
	NEXT_TOKEN;
	while (CurrentToken != TK_END)
	{
		// to allow ";"
		if(CurrentToken == TK_SEMICOLON){
			NEXT_TOKEN;
			continue;
		}
		*tail = ParseExternalDeclaration();
		tail = &(*tail)->next;
		/***********************************************
			The token we encounter here should be in FIRST_ExternalDeclaration,
			Or it is illegal token. 
			If it is legal , nothing is done by SkipTo() .
		 ***********************************************/
		SkipTo(FIRST_ExternalDeclaration, "the beginning of external declaration");
	}

	CloseSourceFile();

	return transUnit;
}

