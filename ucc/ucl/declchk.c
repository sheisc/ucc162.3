#include "ucl.h"
#include "ast.h"
#include "decl.h"
#include "expr.h"
#include "stmt.h"

#include "config.h"


AstFunction CURRENTF;
FunctionSymbol FSYM;


static Table savedIdentifiers,savedTags;

static void CheckDeclarationSpecifiers(AstSpecifiers specs);
static void CheckDeclarator(AstDeclarator dec);

/**
 * Check if the initializer expression is address constant.
 * e.g. 
 * int a;
 * int b = &a;
 */
static AstExpression CheckAddressConstant(AstExpression expr)
{
	AstExpression addr;
	AstExpression p;
	int offset = 0;
	
	if (! IsPtrType(expr->ty))
		return NULL;
	/*************************************************
		int arr[4];
		int main(){
			
				//The result is 
					//4cb060 4cb060 4cb060 .
					//4cb064 4cb070 4cb064 .
		
			printf("%x %x %x .\n", arr,&arr,&arr[0]);
			printf("%x %x %x .\n", arr+1,&arr+1,&arr[0]+1);
			return 0;
		}

	 *************************************************/
	if (expr->op == OP_ADD || expr->op == OP_SUB)
	{
		addr = CheckAddressConstant(expr->kids[0]);
		if (addr == NULL || expr->kids[1]->op != OP_CONST)
			return NULL;		
		expr->kids[0] = addr->kids[0];
		expr->kids[1]->val.i[0] += (expr->op == OP_ADD ? 1 : -1) * addr->kids[1]->val.i[0];
		return expr;
	}
	// &
	if (expr->op == OP_ADDRESS){// &num
		addr = expr->kids[0];
		//PRINT_DEBUG_INFO(("expr->op == OP_ADDRESS"));
	}else{// arr
		addr = expr;
		//PRINT_DEBUG_INFO(("expr->op != OP_ADDRESS"));
	}

	while (addr->op == OP_INDEX || addr->op == OP_MEMBER)
	{
		if (addr->op == OP_INDEX)
		{
			if (addr->kids[1]->op != OP_CONST)
				return NULL;
			offset += addr->kids[1]->val.i[0];	
		}
		else
		{
			Field fld = addr->val.p;

			offset += fld->offset;
		}
		addr = addr->kids[0];
	}

	if (addr->op != OP_ID || (expr->op != OP_ADDRESS && ! addr->isarray && ! addr->isfunc))
		return NULL;

	((Symbol)addr->val.p)->ref++;	
	CREATE_AST_NODE(p, Expression);
	p->op = OP_ADD;
	p->ty = expr->ty;
	p->kids[0] = addr;
	{
		union value val;
		val.i[0] = offset;
		val.i[1] = 0;
		p->kids[1] = Constant(addr->coord, T(INT), val);
	}
	return p;
}
// 
static void CheckInitConstant(AstInitializer init)
{
	InitData initd = init->idata;
	while (initd)
	{
		/****************************************
		InitConstant:
			(1)	OP_CONST		123,  123.45
			(2)	"abcdef"		
					For most cases,
							after function CheckPrimaryExpression(),
							what we have is a special OP_ID.
							But we do have OP_STR when:
								char buf[] = "abcdef";
					We dont' cast "pointer " to integer in 
						CheckInitializerInternal() when if (IsScalarType(ty)).
						to pass CheckAddressConstant(..)
			(3)  int a = 0;	int * ptr = &a;
		 ****************************************/
		 
		if (! (initd->expr->op == OP_CONST || initd->expr->op == OP_STR ||
		       (initd->expr = CheckAddressConstant(initd->expr))))
		{
			Error(&init->coord, "Initializer must be constant");
		}
		initd = initd->next;
	}
	return;
}
// 	"|"		BOR	: bit or		BitField:	bit-field of struct
static AstExpression BORBitField(AstExpression expr1, AstExpression expr2)
{
	AstExpression bor;

	if (expr1->op == OP_CONST && expr2->op == OP_CONST)
	{
		expr1->val.i[0] |= expr2->val.i[0];
		return expr1;
	}

	/*****************************************************
	We can get here when the input file is :
		 struct Data{
			 int high:16;
			 int low:	 16;
		 };
		 
		 struct Data dt = {"abc",12};	// Illegal, but alarmed later.
		 
	 "expr1->op is << , expr2->op is const"
	 *****************************************************/
	// PRINT_DEBUG_INFO(("expr1->op is %s , expr2->op is %s",OPNames[expr1->op],OPNames[expr2->op]));
	CREATE_AST_NODE(bor, Expression);

	bor->coord = expr1->coord;
	bor->ty = expr1->ty;
	bor->op = OP_BITOR;
	bor->kids[0] = expr1;
	bor->kids[1] = expr2;

	return bor;
}

static AstExpression PlaceBitField(Field fld, AstExpression expr)
{
	AstExpression lsh;
	union value val;

	if (expr->op == OP_CONST)
	{
		expr->val.i[0] <<= fld->pos;
		return expr;
	}
	/***********************************************
	We get here when:
		struct Data{
			int high:16;
			int low:	16;
		};
		int abc;
		struct Data dt = {&abc,12};	//	Illegal, but alarmed later.
		"expr->op : &"
	 ***********************************************/
	//PRINT_DEBUG_INFO(("expr->op : %s",OPNames[expr->op]));
	CREATE_AST_NODE(lsh, Expression);

	lsh->coord = expr->coord;
	lsh->ty = expr->ty;
	lsh->op = OP_LSHIFT;
	lsh->kids[0] = expr;
	val.i[1] = 0;
	val.i[0] = fld->pos;
	lsh->kids[1] = Constant(expr->coord, T(INT), val);

	return lsh;
}

static AstInitializer CheckInitializerInternal(InitData *tail, AstInitializer init, Type ty, 
                                               int *offset, int *error)
{
	AstInitializer p;
	int size = 0;
	InitData initd;
	if (IsScalarType(ty))
	{
		p = init;
		while(p->lbrace){
			Warning(&p->coord,"braces around scalar initializer");
			p = (AstInitializer) p->initials;
		}	
		p->expr = Adjust(CheckExpression(p->expr), 1);
		if (! CanAssign(ty, p->expr))
		{
			Error(&init->coord, "Wrong initializer");
			*error = 1;
		}
		else
		{
			/******************************************************
					see 	(1)	examples/str/strAddress.c
						(2)	CheckInitConstant(AstInitializer init)
				If we cast const address to int here,
					CheckInitConstant won't treat it as an const address.
				So we don't do the casting in this case.
			 ******************************************************/
			Type lty = ty, rty = p->expr->ty;
			if( !((IsPtrType(lty) && IsIntegType(rty) || IsPtrType(rty) && IsIntegType(lty))&&
					(lty->size == rty->size))){
				p->expr = Cast(ty, p->expr);		
			}
		}
		ALLOC(initd);
		initd->offset = *offset;
		initd->expr = p->expr;
		initd->next = NULL;
		(*tail)->next = initd;
		*tail = initd;

		return (AstInitializer)init->next;
	}
	else if (ty->categ == UNION)
	{
		p = init->lbrace ? (AstInitializer)init->initials : init;
		/***********************************************
		(1)	Legal
			union Data{	
				char * str;
				double addr;
			};
			union Data dt = {"12345"};		
		(2)	Illegal
			union Data{	
				char * str;
				double addr;
			};
			union Data dt = {"12345"};		
		 ************************************************/
		ty = ((RecordType)ty)->flds->ty;

		p = CheckInitializerInternal(tail, p, ty, offset, error);

		if (init->lbrace)
		{
			if (p != NULL)	{
				Warning(&init->coord," excess elements in union initializer");
			}
			return (AstInitializer)init->next;
		}

		return p;
	}
	else if (ty->categ == ARRAY)
	{
		int start = *offset;
		p = init->lbrace ? (AstInitializer)init->initials : init;

		if (((init->lbrace && ! p->lbrace && p->next == NULL) || ! init->lbrace) &&
		    p->expr->op == OP_STR && ty->bty->categ / 2 == p->expr->ty->bty->categ / 2 &&
		    ty->bty->categ != ARRAY)		
		{
			size = p->expr->ty->size;
			

			if(ty->size == 0){
				ArrayType aty = (ArrayType)ty;
				aty->size = size;
				if(aty->bty->size != 0){
					//PRINT_DEBUG_INFO(("size = %d, %d",aty->bty->size,size));
					aty->len = size / aty->bty->size;
				}
			}
			else if (ty->size == size - p->expr->ty->bty->size){
				//	char str[3] = "abc";	or	int arr[3] = L"123";		
				p->expr->ty->size = size - p->expr->ty->bty->size;
			}
			else if (ty->size < size){
				//	L"123456789"		---->	On Linux, 	size is 40
				//	"abcdef"			---->	size is 7
				//PRINT_DEBUG_INFO(("str->size = %d",size));
				p->expr->ty->size = ty->size;
				Warning(&init->coord,"initializer-string for char/wchar array is too long");
			}

			ALLOC(initd);
			initd->offset = *offset;
			initd->expr = p->expr;
			initd->next = NULL;
			(*tail)->next = initd;
			*tail = initd;

			return (AstInitializer)init->next;
		}

		while (p != NULL)
		{
			p = CheckInitializerInternal(tail, p, ty->bty, offset, error);
			size += ty->bty->size;
			*offset = start + size;
			if (ty->size == size)
				break;
		}
		
		if (ty->size == 0){	
			/********************************************
				calculate the len of array
				int arr[] = {1,2,3,4,5};
			 ********************************************/
			ArrayType aty = (ArrayType) ty;

			if(aty->bty->size != 0 && aty->len == 0){
				//PRINT_CUR_ASTNODE(init);
				aty->len = size/aty->bty->size;
			}
			ty->size = size;			
		}		
	
		if (init->lbrace){
			if(p){
				Warning(&p->coord, "excess elements in array initializer");
			}
			return (AstInitializer)init->next;
		}
		return p;
	}
	else if (ty->categ == STRUCT)
	{
		int start = *offset;
		Field fld = ((RecordType)ty)->flds;
		p = init->lbrace ? (AstInitializer)init->initials : init;
		
		while (fld && p)
		{
			
			*offset = start + fld->offset;
			
			#if 1		// added   see examples/init/struct.c
			if( (IsRecordType(fld->ty) && fld->id == NULL)){
				*offset = start;
			}
			#endif		
			
			//PRINT_DEBUG_INFO(("*offset = %d , start = %d , fld->offset = %d",*offset, start, fld->offset));
			p = CheckInitializerInternal(tail, p, fld->ty, offset, error);
			if (fld->bits != 0)
			{
				(*tail)->expr = PlaceBitField(fld, (*tail)->expr);
			}
			fld = fld->next;
		}
		#if 1		// added
		*offset = start + ty->size;
		//PRINT_DEBUG_INFO(("*offset = %d , start = %d, ty->size= %d",*offset, start, ty->size));
		#endif
		if (init->lbrace)
		{
			if (p != NULL)	{
				Warning(&init->coord, "excess elements in struct initializer");
			}	
			#if 0		// bug ?  see Space()
			PRINT_DEBUG_INFO(("*offset = %d , ty->size= %d",*offset, ty->size));
			*offset = ty->size;
			#endif
			return (AstInitializer)init->next;
		}

		return (AstInitializer)p;
	}

	return init;
}
	
/**********************************************************
	char array[] = "abcdef";
	@expr		"abcdef"
	@bty		char
	@return		0		error
				1		pass
 **********************************************************/
static int CheckCharArrayInit(AstExpression expr, Type bty){
	if(expr->op == OP_STR){
		if( (bty->categ == CHAR || bty->categ == UCHAR) && expr->ty->bty->categ == CHAR){
			return 1;
		}
		if( bty->categ == WCHAR && expr->ty->bty->categ == WCHAR){
			return 1;
		}		
		if( (bty->categ == CHAR || bty->categ == UCHAR) && expr->ty->bty->categ == WCHAR){
			Error(&expr->coord,"char-array initialized from wide string");
			return 0;
		}
		if( bty->categ == WCHAR && expr->ty->bty->categ != WCHAR){
			Error(&expr->coord,"wide character array initialized from non-wide string");
			return 0;
		}
	}
	Error(&expr->coord, "array initializer must be an initializer list");
	return 0;
}

static void CheckInitializer(AstInitializer init, Type ty)
{
	int offset = 0, error = 0;
	struct initData header;
	InitData tail = &header;
	InitData prev, curr;

	header.next = NULL;

	if(ty->categ == ARRAY && !init->lbrace){
		if(!CheckCharArrayInit(init->expr,ty->bty)){
			return ;
		}		
	}
	else if ((ty->categ == STRUCT || ty->categ == UNION) && ! init->lbrace)
	{
		/**************************************************
			struct Data dt;
			struct Data dt2 = dt;		// Legal	in local scope, not in global scope
		 **************************************************/
		init->expr = Adjust(CheckExpression(init->expr), 1);
		if (! CanAssign(ty, init->expr))
		{
			Error(&init->coord, "Wrong initializer");
		}
		else
		{
			ALLOC(init->idata);
			init->idata->expr = init->expr;
			init->idata->offset = 0;
			init->idata->next = NULL;
		}
		return;
	}

	CheckInitializerInternal(&tail, init, ty, &offset, &error);
	if (error)
		return;

	init->idata = header.next;
	prev = NULL;
	curr = init->idata;
	while (curr)
	{
		if (prev != NULL && prev->offset == curr->offset)
		{
			prev->expr = BORBitField(prev->expr, curr->expr);
			prev->next = curr->next;
			curr = curr->next;
		}
		else
		{
			prev = curr;
			curr = curr->next;
		}
	}

}
/**************************************************
	For an id, its type information consists of declaration-specifiers and declarator,
	which are parsing separately during syntax parsing.
	DeriveType(..) combines the separated type information together
	to form a complete type information.
	@tyDryList		type-information from declarator
				POINTER, FUNCTION, ARRAY
	@ty				type-information from declaration-specifiers
				basic type, struct/union, enum,
 **************************************************/
static Type DeriveType(TypeDerivList tyDrvList, Type ty,Coord coord){
	while (tyDrvList != NULL)
	{
		if (tyDrvList->ctor == POINTER_TO)
		{
			ty = Qualify(tyDrvList->qual, PointerTo(ty));
		}
		else if (tyDrvList->ctor == ARRAY_OF)
		{
			if (ty->categ == FUNCTION){ 
				Error(coord, "array of function");
				//	assume ty to be pointer to function
				ty = PointerTo(ty);
			}
			if(IsIncompleteType(ty,!IGNORE_ZERO_SIZE_ARRAY)){
				Error(coord,"array has incomplete element type");
				// assume len to be one, for better error recovery
				((ArrayType) ty)->len = 1;
			}
			if(tyDrvList->len < 0){
				Error(coord,"size of array is negative");
				// assume len to be one, for better error recovery
				((ArrayType) ty)->len = 1;
			}
			ty = ArrayOf(tyDrvList->len, ty);		
		}
		else
		{
			/*********************************************
				when we are here, we are sure that 
					tyDrvList->ctor == FUNCTION_RETURN.
				At this case,
					@ty represents the return value of a function.
				However, in C language ,
					we can't return an array or a function.

				e.g.
					typedef	int INT_ARR[4];
					INT_ARR getArr(){
						INT_ARR a;
						return a;
					}
			 *********************************************/
			if(ty->categ == ARRAY){
				Error(coord,"function cannot return array type");
				// assume ty be pointer to array for better error recovery
				ty = PointerTo(ty);
			}
			if( ty->categ == FUNCTION){
				Error(coord,"function cannot return function type");
				// assume ty be pointer to array for better error recovery
				ty = PointerTo(ty);
			}
			ty = FunctionReturn(ty, tyDrvList->sig);
		}

		tyDrvList = tyDrvList->next;
	}

	return ty;
}

/*********************************************************************
	add item into Vector		astFunctionDeclarator.ids	----  for function without prototype. 
							f(a,b,c)
	@params				
		astFunctionDeclarator.ids
	@id		"a"
	@ty		the type of parameter "a"
	@reg	whether the formal parameter is in register
	@coord				
 *********************************************************************/
static void AddParameter(Vector params, char *id, Type ty, int reg, Coord coord)
{
	Parameter param;
	//	f(a,a,b)		is illegal,  redefinition of parameter.
	FOR_EACH_ITEM(Parameter, param, params)
		if (param->id && param->id == id)
		{
			Error(coord, "Redefine parameter %s", id);
			return;
		}
	ENDFOR
	//
	ALLOC(param);

	param->id = id;
	param->ty = ty;
	param->reg = reg;
	INSERT_ITEM(params, param);
}
//	@funcDec			f(int a, int b, int c)
//	@paramDecl		int a	or 	int b, ...
// 						"int a "	in 	f(int a, int b, int c).
static void CheckParameterDeclaration(AstFunctionDeclarator funcDec,
                                      AstParameterDeclaration paramDecl)
{
	char *id = NULL;
	Type ty = NULL;
	
	CheckDeclarationSpecifiers(paramDecl->specs);
	if (paramDecl->specs->sclass && paramDecl->specs->sclass != TK_REGISTER)
	{
		Error(&paramDecl->coord, "Invalid storage class");
	}
	//	right now, the @ty maybe incomplete, we will use DeriveType to form a complete type later.
	ty = paramDecl->specs->ty;
	CheckDeclarator(paramDecl->dec);
	// f(void)		?
	if (paramDecl->dec->id == NULL && paramDecl->dec->tyDrvList == NULL &&
	    ty->categ == VOID && LEN(funcDec->sig->params) == 0)
	{
		if (paramDecl->next || ty->qual || paramDecl->specs->sclass)
		{
			Error(&paramDecl->coord, "'void' must be the only parameter");
			paramDecl->next = NULL;
		}
		return;
	}

	ty = DeriveType(paramDecl->dec->tyDrvList, ty,&paramDecl->coord);
	// when we are here, the type information for parameter is completer now.
	if (ty != NULL)
		ty = AdjustParameter(ty);

	if (ty == NULL){	
		Error(&paramDecl->coord, "Illegal parameter type");
		return;
	}
	if(funcDec->partOfDef && IsIncompleteType(ty,IGNORE_ZERO_SIZE_ARRAY)){
		Error(&paramDecl->coord,"parameter has incomplete type");
	}	

	id = paramDecl->dec->id;
	// only when we declare a function, the parameter can be anonymous.
	//	f(int,int,int);
	//	but in a definition,
	//	f(int,int ,int ){}		is illegal.
	if (id == NULL && funcDec->partOfDef)
	{
		Error(&paramDecl->coord, "Expect parameter name");
		return;
	}
	
	AddParameter(funcDec->sig->params, id, ty, 
		paramDecl->specs->sclass == TK_REGISTER, &paramDecl->coord);
}
//	"int a, int b , int c "	in 	f(int a, int b, int c).
static void CheckParameterTypeList(AstFunctionDeclarator funcDec)
{
	AstParameterTypeList paramTyList = funcDec->paramTyList;
	AstParameterDeclaration paramDecl;
	
	paramDecl = (AstParameterDeclaration)paramTyList->paramDecls;
	while (paramDecl)
	{
		CheckParameterDeclaration(funcDec, paramDecl);
		paramDecl = (AstParameterDeclaration)paramDecl->next;
	}
	funcDec->sig->hasEllipsis = paramTyList->ellipsis;
}
//	int f(int a, int b, int c)
static void CheckFunctionDeclarator(AstFunctionDeclarator dec)
{
	AstFunctionDeclarator funcDec = (AstFunctionDeclarator)dec;


	CheckDeclarator(funcDec->dec);
	// see examples/scope/function.c
	EnterParameterList();

	ALLOC(funcDec->sig);
	/********************************************
		int f(int a, int b, int c);
			not
		int f(a,b,c).
	 ********************************************/
	funcDec->sig->hasProto = funcDec->paramTyList != NULL;
	funcDec->sig->hasEllipsis = 0;
	funcDec->sig->params = CreateVector(4);

	if (funcDec->sig->hasProto)
	{
		/*******************************
			int f(int a,int b,int c){
				//......
			}			
		 *******************************/	
		CheckParameterTypeList(funcDec);
	}
	else if (funcDec->partOfDef)
	{	
		/*******************************
			int f(a,b,c)	-------------->  the type for a,b,c is NULL.
				int a,b;double c;{
				//......
			}
			
		 *******************************/	
		char *id;
		FOR_EACH_ITEM(char*, id, funcDec->ids)
			AddParameter(funcDec->sig->params, id, NULL, 0, &funcDec->coord);
		ENDFOR 
	}
	else if (LEN(funcDec->ids))
	{	// void f(a,b,c);		see examples/function/oldstyle.c
		Error(&funcDec->coord, "Identifier list should be in definition.");
	}
	ALLOC(funcDec->tyDrvList);
	funcDec->tyDrvList->ctor = FUNCTION_RETURN;
	funcDec->tyDrvList->sig  = funcDec->sig;
	funcDec->tyDrvList->next = funcDec->dec->tyDrvList;
	funcDec->id = funcDec->dec->id;

	
	if(funcDec->partOfDef){
		SaveParameterListTable();
	}
	LeaveParemeterList();
}
//	int arr[4];
static void CheckArrayDeclarator(AstArrayDeclarator arrDec)
{
	CheckDeclarator(arrDec->dec);
	/********************************
		struct Data{
			....
			int a[];	----> legal. 	when  arrDec->expr is NULL.
		}
	 ********************************/
	if (arrDec->expr)
	{
		if ((arrDec->expr = CheckConstantExpression(arrDec->expr)) == NULL)
		{
			Error(&arrDec->coord, "The size of the array must be integer constant.");
		}
	}
	
	ALLOC(arrDec->tyDrvList);
	arrDec->tyDrvList->ctor = ARRAY_OF;
	arrDec->tyDrvList->len  = arrDec->expr ? arrDec->expr->val.i[0] : 0;
	arrDec->tyDrvList->next = arrDec->dec->tyDrvList;
	arrDec->id = arrDec->dec->id;
}
//	int	*a[5];
static void CheckPointerDeclarator(AstPointerDeclarator ptrDec)
{
	int qual = 0;
	AstToken tok = (AstToken)ptrDec->tyQuals;
	// we want to create typeDerivList object for the successor of @ptrDec first.
	CheckDeclarator(ptrDec->dec);
	// When we are here, we are sure that the typeDerivList Object has been created
	// for the successor of @ptrDec.		So we can do this later:
	//	"ptrDec->tyDrvList->next = ptrDec->dec->tyDrvList;"
	while (tok)
	{
		qual |= tok->token == TK_CONST ? CONST : VOLATILE;
		tok = (AstToken)tok->next;
	}

	ALLOC(ptrDec->tyDrvList);
	ptrDec->tyDrvList->ctor = POINTER_TO;
	ptrDec->tyDrvList->qual = qual;
	ptrDec->tyDrvList->next = ptrDec->dec->tyDrvList;
	ptrDec->id = ptrDec->dec->id;
}
/***********************************************************
	int * a[5];
	(1)
	During syntax parsing, we have constructed an AST

	astPointerDeclarator
		TypeDerivList 	tyDrvList;	
		dec				----->		astArrayDeclarator
										dec			-------->		astDeclarator
										tyDrvList					"a"
																	tyDrvList

	(2)	During semantics checking,
		we allocate Typderiv object for astArrayDeclarator.tyDrvList first,
		then for astPointerDerivList.tyDrvList

		astPointerDerivList.tyDrvList		---->   astArrayDeclarator.tyDrvList	--->NULL
 ***********************************************************/
static void CheckDeclarator(AstDeclarator dec)
{
	switch (dec->kind)
	{
	case NK_NameDeclarator:
		break;

	case NK_ArrayDeclarator:
		CheckArrayDeclarator((AstArrayDeclarator)dec);
		break;

	case NK_FunctionDeclarator:
		CheckFunctionDeclarator((AstFunctionDeclarator)dec);
		break;

	case NK_PointerDeclarator:
		CheckPointerDeclarator((AstPointerDeclarator)dec);
		break;

	default:
		assert(0);
	}
}
/*******************************************
	struct Data{
		int 	a;	-------------------->  This is a struct-declarator.
	}
	@rty 	the type of Data, RecordType
	@stDec			"a"
	@fty		"int"		field type
			the "true type" of 'a' is combination of "int" and 'declarator'
			for example,
					int 	a[3];
 *******************************************/
static void CheckStructDeclarator(Type rty, AstStructDeclarator stDec, Type fty)
{
	char *id = NULL;
	int bits = 0;

	if (stDec->dec != NULL)
	{
		CheckDeclarator(stDec->dec);
		id = stDec->dec->id;
		fty = DeriveType(stDec->dec->tyDrvList, fty, &stDec->coord);
	}

	/*********************************************
		(1) 
				struct Data{
					a;	-----------	fty == NULL
				}
		(2)
				struct Data{
					void f(){	-----------	fty-categ == FUNCTION
					}
				}
		(3)		(fty->size == 0 && fty->categ != ARRAY)
				only  flexible array at the end of a struct is legal.

		Bug:
			typedef struct{
				int b;
				struct{
				}a;	--------------> fty->size == 0
				int c;
			}Data;
	 *********************************************/
	if (fty == NULL || fty->categ == FUNCTION 
		|| (fty->size == 0 && (IsIncompleteRecord(fty) || IsIncompleteEnum(fty))))
	{
		Error(&stDec->coord, "illegal type");
		return;
	}

	if (((RecordType)rty)->hasFlexArray && rty->categ == STRUCT){
		Error(&stDec->coord, "the flexible array must be the last member");
		//return;
	}		
	if (id && LookupField(rty, id))
	{
		Error(&stDec->coord, "member redefinition");
		return;
	}
	/*****************************************
		struct Data{
			int a:123;		---------- "123" 	is the stDec->expr
		}
	 *****************************************/
	if (stDec->expr)
	{
		stDec->expr = CheckConstantExpression(stDec->expr);
		if (stDec->expr == NULL)
		{
			Error(&stDec->coord, "The width of bit field should be integer constant.");
		}
		else if (id && stDec->expr->val.i[0] == 0)
		{
			//	"int:0;" is a legal and special case
			Error(&stDec->coord, "bit field's width should not be 0");
		}
		if(	fty->categ >= CHAR && fty->categ <= ULONG){
			// The original UCC only allow bit-field with INT or UINT type,
			// In order to allow char/short/.../,
			// we promote char /short to int here, to be consistent with the code in EndRecord().
			// But the memory layout maybe different from cl.exe on Windows.
			// It seems that GCC/Clang also use the trick here.
			// Rewriting EndRecord() may be another solution.
			// But just keep it simple :)
			fty = IsUnsigned(fty)?T(UINT):T(INT);
		}else{
			Error(&stDec->coord, "Bit field must be integer type.");
			fty = T(INT);
		}
		bits = stDec->expr ? stDec->expr->val.i[0] : 0;
		if (bits > T(INT)->size * 8)
		{
			Error(&stDec->coord, "Bit field's width exceeds");
			bits = 0;
		}
		if(bits < 0){
			Error(&stDec->coord, "bit-field has negative width");
			bits = 0;
		}
	}
	AddField(rty, id, fty, bits);
}
/*********************************************************
	struct-declaration:
		"int a;" or "int b:2;"
			in 
		struct Data{
			int;		----->		anonymous struct-declaration
			int a;
			int b:2;
			....
		}

		@rty		is a object of RecordType.
 *********************************************************/
static void CheckStructDeclaration(AstStructDeclaration stDecl, Type rty)
{
	AstStructDeclarator stDec;

	CheckDeclarationSpecifiers(stDecl->specs);
	stDec = (AstStructDeclarator)stDecl->stDecs;
	/*****************************************
		struct Data{
			int;		----->		anonymous struct-declaration	
			......
		}
	 *****************************************/
	if (stDec == NULL)
	{
		// see  examples/struct/anonymousField.c
		if(IsRecordSpecifier(stDecl->specs->tySpecs)){
			AstStructSpecifier spec = (AstStructSpecifier) stDecl->specs->tySpecs;
			if(spec->id == NULL){
				AddField(rty, NULL, stDecl->specs->ty, 0);
				return;
			}
		}
		Warning(&stDecl->coord,"declaration does not declare anything");
		return;
	}
	/**************************
		struct Data{
			int c, d;			
		}
	 **************************/
	while (stDec)
	{
		CheckStructDeclarator(rty, stDec, stDecl->specs->ty);
		stDec = (AstStructDeclarator)stDec->next;
	}  
}

/**************************************************
	struct-or-union-specifier:
		struct-or-union id(opt) {struct-declaration-list}
		struct-or-union id

	struct Data{
		int a;
		int b:2;
		....
	}
 **************************************************/ 
static Type CheckStructOrUnionSpecifier(AstStructSpecifier stSpec)
{
	int categ = (stSpec->kind == NK_StructSpecifier) ? STRUCT : UNION;
	Symbol tag;
	Type ty;
	AstStructDeclaration stDecl;
	/*****************************************************
		call StartRecord and AddTag(..) to add a new symbol when necessary 
	 *****************************************************/
	if (stSpec->id != NULL && !stSpec->hasLbrace)
	{	// struct-or-union id
		tag = LookupTag(stSpec->id);
		if (tag == NULL)
		{
			ty = StartRecord(stSpec->id, categ);
			tag = AddTag(stSpec->id, ty,&stSpec->coord);
		}
		else if (tag->ty->categ != categ)
		{
			Error(&stSpec->coord, "Inconsistent tag declaration.");
		}
		
		return tag->ty;
	}
	else if (stSpec->id == NULL && stSpec->hasLbrace)
	{	// struct-or-union	{struct-declaration-list}
		ty = StartRecord(NULL, categ);
		// Anytime, If  the lbrace of struct/union exists, it is treated a complete type.
		((RecordType)ty)->complete = 1;
		goto chk_decls;
	}
	else if (stSpec->id != NULL && stSpec->hasLbrace)
	{	// struct-or-union	id	{struct-declaration-list}
		tag = LookupTag(stSpec->id);
		
		if (tag == NULL || tag->level < Level)
		{	// If it has not been declared yet, or has but in outer-scope.
			ty = StartRecord(stSpec->id, categ);
			((RecordType)ty)->complete = 1;
			AddTag(stSpec->id, ty,&stSpec->coord);
		}
		else if (tag->ty->categ == categ && IsIncompleteRecord(tag->ty))
		{
			ty = tag->ty;
			((RecordType)ty)->complete = 1;
		}
		else
		{
			if(tag->ty->categ != categ){
				Error(&stSpec->coord, "\'%s\'defined as wrong kind of tag.", stSpec->id);
			}else{
				Error(&stSpec->coord, "redefinition of \'%s %s\'.", 
						GetCategName(categ),stSpec->id);
			}
			return tag->ty;
		}
		goto chk_decls;
	}
	else
	{	// struct-or-union;		illegal & already alarmed during syntax parsing		
		ty = StartRecord(NULL, categ);
		EndRecord(ty,&stSpec->coord);
		return ty;
	}

chk_decls:
	stDecl = (AstStructDeclaration)stSpec->stDecls;

	while (stDecl)
	{
		CheckStructDeclaration(stDecl, ty);
		stDecl = (AstStructDeclaration)stDecl->next;
	}
	// calculate the size of the struct and other type-informations.
	EndRecord(ty,&stSpec->coord);
	return ty;

}
/*******************************************************
	enum COLOR{
		RED,
		GREEN = 3,
		BLUE
	}
	
 *******************************************************/
//see examples/enum/enumerator.c
static int CheckEnumRedeclaration(AstEnumerator enumer){
	Symbol sym;
	sym = LookupID(enumer->id);
	if(sym  && sym->level == Level){
		if(sym->kind != SK_EnumConstant){
			Error(&enumer->coord,"\'%s\'redeclared as different kind of symbol",enumer->id);		
		}else{
			Error(&enumer->coord,"redeclaration of enumerator \'%s\'",enumer->id);
		}
		return 1;
	}else{
		return 0;
	}
}

static int CheckEnumerator(AstEnumerator enumer, int last, Type ty)
{

	CheckEnumRedeclaration(enumer);
	if (enumer->expr == NULL)
	{	// e.g.	RED		
		AddEnumConstant(enumer->id, ty, last + 1,&enumer->coord);
		return last + 1;
	}
	else
	{	// e.g.	GREEN = 3
		enumer->expr = CheckConstantExpression(enumer->expr);
		if (enumer->expr == NULL)
		{
			Error(&enumer->coord, "The enumerator value must be integer constant.");
			AddEnumConstant(enumer->id, ty, last + 1,&enumer->coord);
			return last + 1;
		}
		
		AddEnumConstant(enumer->id, ty, enumer->expr->val.i[0],&enumer->coord);

		return enumer->expr->val.i[0];
	}
	
}
/******************************************************
	enum COLOR{
		RED,GREEN=2,BLUE
	}
 ******************************************************/
static Type CheckEnumSpecifier(AstEnumSpecifier enumSpec)
{

		AstEnumerator enumer;
		Symbol tag;
		Type ty;
		int last;
		/******************************************************
			Because " enum {}" is illegal, but "struct {}" is legal.
			So we don't need hasLbrace.
		 ******************************************************/
		if (enumSpec->id == NULL && enumSpec->enumers == NULL){
			//	enum;		illegal , already alarmed during syntax parsing			
			return T(INT);
		}
		
		if (enumSpec->id != NULL && enumSpec->enumers == NULL)
		{	//	enum COLOR ...				
			tag = LookupTag(enumSpec->id);
			if(tag == NULL){
				tag = AddTag(enumSpec->id, Enum(enumSpec->id),&enumSpec->coord); 		
			}else if(tag->ty->categ != ENUM){
				Error(&enumSpec->coord, "Inconsistent tag declaration.");
				return T(INT);
			}		
			return tag->ty; 			
		}
		else if (enumSpec->id == NULL && enumSpec->enumers != NULL)
		{	//	enum {RED,GREEN,BLUE} ...
			ty = T(INT);			
		}
		else	
		{	//	enum Color{RED,GREEN,BLUE}
			EnumType ety;
			tag = LookupTag(enumSpec->id);
			if (tag == NULL || tag->level < Level){
				tag = AddTag(enumSpec->id, Enum(enumSpec->id),&enumSpec->coord);
			}
			else if (tag->ty->categ != ENUM)
			{
				Error(&enumSpec->coord, "\'%s\' defined as a wrong kind of tag.",enumSpec->id);
				return T(INT);
			}
			assert(tag && tag->level == Level && tag->ty->categ == ENUM);		
			ety = (EnumType) tag->ty;
			if(ety ->complete){
				Error(&enumSpec->coord, "redefinition of \'enum %s\' .", enumSpec->id); 
				return tag->ty;
			}else{
				ety->complete = 1;
			}		
			ty = tag->ty;			
		}
		//
		enumer = (AstEnumerator)enumSpec->enumers;
		last = -1;
		while (enumer)
		{
			last = CheckEnumerator(enumer, last, ty);
			enumer = (AstEnumerator)enumer->next;
		}
	
		return ty;

}

static void CheckDeclarationSpecifiers(AstSpecifiers specs)
{
	AstToken tok;
	AstNode p;
	Type ty;
	int size = 0, sign = 0;
	int signCnt = 0, sizeCnt = 0, tyCnt = 0;
	int qual = 0;
	//storage-class-specifier:		extern	, auto,	static, register, ... 
	tok = (AstToken)specs->stgClasses;
	if (tok)
	{
		if (tok->next)
		{
			Error(&specs->coord, "At most one storage class");
		}
		specs->sclass = tok->token;
	}
	//type-qualifier:	const, volatile
	tok = (AstToken)specs->tyQuals;
	while (tok)
	{
		qual |= (tok->token == TK_CONST ? CONST : VOLATILE);
		tok = (AstToken)tok->next;
	}
	//type-specifier:	int,double, struct ..., union ..., ...
	p = specs->tySpecs;
	while (p)
	{
		if (p->kind == NK_StructSpecifier || p->kind == NK_UnionSpecifier)
		{
			ty = CheckStructOrUnionSpecifier((AstStructSpecifier)p);
			tyCnt++;
		}
		else if (p->kind == NK_EnumSpecifier)
		{
			ty = CheckEnumSpecifier((AstEnumSpecifier)p);
			tyCnt++;
		}
		else if (p->kind == NK_TypedefName)
		{
			Symbol sym = LookupID(((AstTypedefName)p)->id);

			assert(sym->kind == SK_TypedefName);
			ty = sym->ty;
			tyCnt++;
		}
		else 
		{
			tok = (AstToken)p;

			switch (tok->token)
			{
			case TK_SIGNED:
			case TK_UNSIGNED:
				sign = tok->token;
				signCnt++;
				break;

			case TK_SHORT:
			case TK_LONG:
				if (size == TK_LONG && sizeCnt == 1)
				{
					size = TK_LONG + TK_LONG;
				}
				else
				{
					size = tok->token;
					sizeCnt++;
				}
				break;

			case TK_CHAR:
				ty = T(CHAR);
				tyCnt++;
				break;

			case TK_INT:
				ty = T(INT);
				tyCnt++;
				break;

			case TK_INT64:
				ty = T(INT);
				size = TK_LONG + TK_LONG;
				sizeCnt++;
				break;

			case TK_FLOAT:
				ty = T(FLOAT);
				tyCnt++;
				break;

			case TK_DOUBLE:
				ty = T(DOUBLE);
				tyCnt++;
				break;

			case TK_VOID:
				ty = T(VOID);
				tyCnt++;
				break;
			}
		}
		p = p->next;
	}
	//When only signed or unsigned or nothing is specified, the default type is int.
	ty = tyCnt == 0 ? T(INT) : ty;
	/*********************************************
		typedef	int double;
			------------		tyCnt is 2.
	 *********************************************/
	if (sizeCnt > 1 || signCnt > 1 || tyCnt > 1)
		goto err;

	if (ty == T(DOUBLE) && size == TK_LONG)
	{
		ty = T(LONGDOUBLE);
		size = 0;
	}
	else if (ty == T(CHAR))
	{
		sign = (sign == TK_UNSIGNED);
		ty = T(CHAR + sign);
		sign = 0;
	}

	if (ty == T(INT))
	{
		sign = (sign == TK_UNSIGNED);

		switch (size)
		{
		case TK_SHORT:
			ty = T(SHORT + sign);
			break;

		case TK_LONG:
			ty = T(LONG + sign);
			break;

		case TK_LONG + TK_LONG:
			ty = T(LONGLONG + sign);
			break;

		default:
			assert(size == 0);
			ty = T(INT + sign);
			break;
		}
		
	}
	else if (size != 0 || sign != 0)
	{
		goto err;
	}

	specs->ty = Qualify(qual, ty);
	return;

err:
	Error(&specs->coord, "Illegal type specifier.");
	specs->ty = T(INT);
	return;
}
/******************************************************************
For each declaration, call CheckTypeDef(). If the declaration is a type definition, look
up if the typedef name exists, if inexistent new typedef name will be added, otherwise
modify level to be the minimum nesting level. If the variable defined by this declaration
was already defined as a typedef name in outer scope, mark the typedef name as overloaded
and add it to OverloadNames.
When a compound statement ends, reset all the typedef names' overload status
in OverloadNames.
 ******************************************************************/
static void CheckTypedef(AstDeclaration decl)
{
	AstInitDeclarator initDec;
	Type ty;
	Symbol sym;

	initDec = (AstInitDeclarator)decl->initDecs;
	while (initDec)
	{
		CheckDeclarator(initDec->dec);
		if (initDec->dec->id == NULL){			
			goto next;
		}
		ty = DeriveType(initDec->dec->tyDrvList, decl->specs->ty,&initDec->coord);
		if (ty == NULL)
		{
			Error(&initDec->coord, "Illegal type");
			ty = T(INT);
		}
		// typedef int INT32 = 200;	-----> error
		if (initDec->init)
		{
			Error(&initDec->coord, "Can't initialize typedef name");
		}

		sym = LookupID(initDec->dec->id);
		// see  examples/typedef/different.c
		if (sym && sym->level == Level && (sym->kind != SK_TypedefName || ! IsCompatibleType(ty, sym->ty)))
		{
			Error(&initDec->coord, "Redeclaration of %s", initDec->dec->id);
		}
		else
		{
			AddTypedefName(initDec->dec->id, ty,&initDec->coord);
		}
next:
		initDec = (AstInitDeclarator)initDec->next;
	}
}
// 
Type CheckTypeName(AstTypeName tname)
{
	Type ty;

	CheckDeclarationSpecifiers(tname->specs);
	CheckDeclarator(tname->dec);
	ty = DeriveType(tname->dec->tyDrvList, tname->specs->ty,&tname->coord);
	if (ty == NULL)
	{
		Error(&tname->coord, "Illegal type");
		ty = T(INT);
	}
	return ty;
}
/****************************************************
	check local declaration in compound-statement

		{
			int local_var = 100;		------ one local declaration
			....
		}
 *****************************************************/
void CheckLocalDeclaration(AstDeclaration decl, Vector v)
{
	AstInitDeclarator initDec;
	Type ty;
	int sclass;
	Symbol sym;
	
	CheckDeclarationSpecifiers(decl->specs);
	/**********************************************
		(1)	storage-specifier-class  is TK_TYPEDEF
				 typedef 	int  INT_ARR[4];
		(2)	But for the following code, the storage-specifier-class is not existing.  ?
			INT_ARR	arr;			
	 ***********************************************/	
	if (decl->specs->sclass == TK_TYPEDEF)
	{
		// PRINT_CUR_POS(0);
		CheckTypedef(decl);
		return;
	}
	ty = decl->specs->ty;
	sclass = decl->specs->sclass;
	// the local variable is AUTO default.
	if (sclass == 0)
	{
		sclass = TK_AUTO;
	}
	initDec = (AstInitDeclarator)decl->initDecs;
	while (initDec)
	{
		CheckDeclarator(initDec->dec);
		if (initDec->dec->id == NULL){	// 	int ;		
			goto next;
		}
		ty = DeriveType(initDec->dec->tyDrvList, decl->specs->ty,&initDec->coord);
		if (ty == NULL)
		{
			Error(&initDec->coord, "Illegal type");
			ty = T(INT);
		}
		if (IsFunctionType(ty))
		{
			/**************************************************
				void f1(void){
				    static void g(void); // illegal to declare static function in local scope in C89
				}
				But in VS2008, it is OK.
			 **************************************************/
			if (sclass == TK_STATIC)
			{
				Error(&decl->coord, "can't specify static for block-scope function");
			}
			if (initDec->init != NULL)
			{
				Error(&initDec->coord, "please don't initialize function");
			}
			if ((sym = LookupID(initDec->dec->id)) == NULL)
			{
				sym = AddFunction(initDec->dec->id, ty, TK_EXTERN,&initDec->coord);
			}
			else if (! IsCompatibleType(sym->ty, ty))
			{
				Error(&decl->coord, "Incompatible with previous declaration");
			}
			else
			{
				sym->ty = CompositeType(sym->ty, ty);
			}

			goto next;
		}
		/***********************************************
			void f(void){
				extern int a = 3;		------	error
			}
		 ************************************************/
		if (sclass == TK_EXTERN && initDec->init != NULL)
		{
			Error(&initDec->coord, "can't initialize extern variable");
			initDec->init = NULL;
		}		
		if ((sym = LookupID(initDec->dec->id)) == NULL || sym->level < Level)
		{
			// here , we check local variables or static variables in function-definition.
			VariableSymbol vsym;
			// see examples/declaration/extern.c
			if(sclass == TK_EXTERN && sym && !IsCompatibleType(sym->ty, ty)){
				Error(&decl->coord, "redefinition of \'%s\' with a different type",initDec->dec->id);
			}
			vsym = (VariableSymbol)AddVariable(initDec->dec->id, ty, sclass,&initDec->coord);
			if (initDec->init)
			{
				CheckInitializer(initDec->init, ty);
				if (sclass == TK_STATIC)
				{
					CheckInitConstant(initDec->init);
				}
				else
				{
					INSERT_ITEM(v, vsym);
				}
				vsym->idata = initDec->init->idata;
			}
		}
		else if (! (sym->sclass == TK_EXTERN && sclass == TK_EXTERN && IsCompatibleType(sym->ty, ty)))
		{
			if(ty->categ == sym->ty->categ){
				Error(&decl->coord,"redefinition of \'%s\' ",initDec->dec->id);
			}else{
				Error(&decl->coord,"redefinition of \'%s\' as different kind of symbol", initDec->dec->id);
			}
		}
next:
		if(sclass != TK_EXTERN && IsIncompleteType(ty,!IGNORE_ZERO_SIZE_ARRAY)){
			Error(&initDec->coord,"variable \'%s\' has incomplete type", initDec->dec->id);	
		}
		initDec = (AstInitDeclarator)initDec->next;
	}

}
/*****************************************************
	declaration:
			declaration-specifiers	init-declaration-list(opt)
 *****************************************************/
static void CheckGlobalDeclaration(AstDeclaration decl)
{
	AstInitDeclarator initDec;
	Type ty;
	Symbol sym;
	int sclass;

	CheckDeclarationSpecifiers(decl->specs);
	// typedef 	int INT_ARR[4];
	if (decl->specs->sclass == TK_TYPEDEF)
	{
		CheckTypedef(decl);
		return;
	}

	ty = decl->specs->ty;
	sclass = decl->specs->sclass;
	// the storage-class could only be EXTERN or STATIC for global variable.
	if (sclass == TK_REGISTER || sclass == TK_AUTO)
	{
		Error(&decl->coord, "Invalid storage class");
		sclass = TK_EXTERN;
	}
	// check init-declarator list.
	initDec = (AstInitDeclarator)decl->initDecs;
	while (initDec)
	{
		CheckDeclarator(initDec->dec);
		//	int	;	----->		anonymous
		if (initDec->dec->id == NULL)
			goto next;

		ty = DeriveType(initDec->dec->tyDrvList, decl->specs->ty,&initDec->coord);
		if (ty == NULL)
		{
			Error(&initDec->coord, "Illegal type");
			ty = T(INT);
		}
		// 
		if (IsFunctionType(ty))
		{
			if (initDec->init)
			{
				Error(&initDec->coord, "please don't initalize function");
			}		
			if ((sym = LookupID(initDec->dec->id)) == NULL )
			{
				sym = AddFunction(initDec->dec->id, ty, sclass == 0 ? TK_EXTERN : sclass,&initDec->coord);
			}
			else
			{
				if (sym->sclass == TK_EXTERN && sclass == TK_STATIC)
				{
					Error(&decl->coord, "Conflict linkage of function %s", initDec->dec->id);
				}

				if (! IsCompatibleType(ty, sym->ty))
				{
					Error(&decl->coord, "Incomptabile with previous declaration");
				}
				else
				{
					sym->ty = CompositeType(ty, sym->ty);
				}
			}
			goto next;
		}
		/************************************************
			Check for global variables
		 ************************************************/
		if ((sym = LookupID(initDec->dec->id)) == NULL)
		{
			sym = AddVariable(initDec->dec->id, ty, sclass,&initDec->coord);
		}

		if (initDec->init)
		{
			CheckInitializer(initDec->init, ty);
			CheckInitConstant(initDec->init);
		}		
		
		sclass = sclass == TK_EXTERN ? sym->sclass : sclass;

		if ((sclass == 0 && sym->sclass == TK_STATIC) || (sclass != 0 && sym->sclass != sclass))
		{
			Error(&decl->coord, "Conflict linkage of %s", initDec->dec->id);
		}
		if (sym->sclass == TK_EXTERN){
			//PRINT_DEBUG_INFO(("% d , %d",sym->sclass,sclass));
			sym->sclass = sclass;			
		}

		if (! IsCompatibleType(ty, sym->ty))
		{
			Error(&decl->coord, "Incompatible with previous definition", initDec->dec->id);
			goto next;
		}
		else
		{
			sym->ty = CompositeType(sym->ty, ty);
		}

		if (initDec->init)
		{
			if (sym->defined)
			{
				Error(&initDec->coord, "redefinition of %s", initDec->dec->id);
			}
			else
			{
				AsVar(sym)->idata = initDec->init->idata;
				sym->defined = 1;
			}

		}
next:
		initDec = (AstInitDeclarator)initDec->next;
	}
	
}
// declaration-list(opt)  in old-style function-definition
static void CheckIDDeclaration(AstFunctionDeclarator funcDec, AstDeclaration decl)
{
	Type ty, bty;
	int sclass;
	AstInitDeclarator initDec;
	Parameter param;
	Vector params = funcDec->sig->params;

	CheckDeclarationSpecifiers(decl->specs);
	sclass = decl->specs->sclass;
	bty = decl->specs->ty;
	if (! (sclass == 0 || sclass == TK_REGISTER))
	{
		Error(&decl->coord, "Invalid storage class");
		sclass = 0;
	}

	initDec = (AstInitDeclarator)decl->initDecs;
	while (initDec)
	{
		if (initDec->init)
		{
			Error(&initDec->coord, "Parameter can't be initialized");
		}
		CheckDeclarator(initDec->dec);
		if (initDec->dec->id == NULL)
			goto next_dec;

		FOR_EACH_ITEM(Parameter, param, params)
			if (param->id == initDec->dec->id)
				goto ok;
		ENDFOR
		Error(&initDec->coord, "%s is not from the identifier list", initDec->dec->id);
		goto next_dec;

ok:
		ty = DeriveType(initDec->dec->tyDrvList, bty,&initDec->coord);
		if (ty == NULL)
		{
			Error(&initDec->coord, "Illegal type");
			ty = T(INT);
		}
		if (param->ty == NULL)
		{
			param->ty = ty;
			param->reg = sclass == TK_REGISTER;
		}
		else
		{
			Error(&initDec->coord, "Redefine parameter %s", param->id);
		}
next_dec:
		initDec = (AstInitDeclarator)initDec->next;
	}
}
/******************************************************
	function-definition:
		declaration-specifiers(opt)	declarator  declaration-list(opt)	compound-statement
 *******************************************************/
void CheckFunction(AstFunction func)
{
	Symbol sym;
	Type ty;
	int sclass;
	Label label;
	AstNode p;
	Vector params;
	Parameter param;
	
	func->fdec->partOfDef = 1;
	// check declaration-specifiers
	CheckDeclarationSpecifiers(func->specs);
	//The default storage-class of function definition is extern.
	if ((sclass = func->specs->sclass) == 0)
	{
		sclass = TK_EXTERN;
	}
	// check declarator
	CheckDeclarator(func->dec);
	// "double a;int b;"  in 	"void g(a,b) double a;int b;{}"
	p = func->decls;
	while (p)
	{
		CheckIDDeclaration(func->fdec, (AstDeclaration)p);
		p = p->next;
	}

	params = func->fdec->sig->params;
	FOR_EACH_ITEM(Parameter, param, params)
		if (param->ty == NULL)
			param->ty = T(INT);
	ENDFOR
		
	ty = DeriveType(func->dec->tyDrvList, func->specs->ty,&func->coord);
	if (ty == NULL)
	{
		Error(&func->coord, "Illegal function type");
		ty = DefaultFunctionType;
	}	
	sym = LookupID(func->dec->id);
	if (sym == NULL)	
	{
		func->fsym = (FunctionSymbol)AddFunction(func->dec->id, ty, sclass,&func->coord);
	}
	else if (sym->ty->categ != FUNCTION)
	{
		Error(&func->coord, "Redeclaration as a function");
		func->fsym = (FunctionSymbol)AddFunction(func->dec->id, ty, sclass,&func->coord);
	}
	else
	{
		func->fsym = (FunctionSymbol)sym;
		if (sym->sclass == TK_EXTERN && sclass == TK_STATIC)
		{
			Error(&func->coord, "Conflict function linkage");
		}
		if (! IsCompatibleType(ty, sym->ty))
		{
			Error(&func->coord, "Incompatible with previous declaration");
			sym->ty = ty;
		}
		else
		{
			sym->ty = CompositeType(ty, sym->ty);
		}
		if (func->fsym->defined)
		{
			Error(&func->coord, "Function redefinition");
		}
	}
	func->fsym->defined = 1;
	func->loops = CreateVector(4);
	func->breakable = CreateVector(4);
	func->swtches = CreateVector(4);

	CURRENTF = func;
	FSYM = func->fsym;
	
	RestoreParameterListTable();

	{
		Vector v= ((FunctionType)ty)->sig->params;
		
		FOR_EACH_ITEM(Parameter, param, v)
			AddVariable(param->id, param->ty, param->reg ? TK_REGISTER : TK_AUTO,&func->coord);
		ENDFOR

		FSYM->locals = NULL;
		FSYM->lastv = &FSYM->locals;
	}
	CheckCompoundStatement(func->stmt);
	ExitScope();
	// Referencing an undefined label is considered as an error.
	label = func->labels;
	while (label)
	{
		if (! label->defined)
		{
			Error(&label->coord, "Undefined label");
		}
		label = label->next;
	}
	// only when return type is void, the return value can be omitted
	if (ty->bty != T(VOID) && ! func->hasReturn)
	{
		Warning(&func->coord, "missing return value");
	}
}

static void CheckIncompleteGlobals(void){
	Symbol p = Globals;
	while(p){
		if(p->sclass != TK_EXTERN && IsZeroSizeArray(p->ty)){			
			Warning(p->pcoord, "array \'%s\' assumed to have one element",p->name);
			p->ty->size = p->ty->bty->size;
		}
		if(p->sclass != TK_EXTERN && IsIncompleteType(p->ty,!IGNORE_ZERO_SIZE_ARRAY)){				
			Error(p->pcoord,"storage size of \'%s\' is unknown", p->name);				
		}
		p = p->next;
	}
}

void CheckTranslationUnit(AstTranslationUnit transUnit)
{
	AstNode p;
	Symbol f;
	
	p = transUnit->extDecls;
	/*****************************************
		Check every 
		(1)	Function
		(2) GlobalDeclaration
	 *****************************************/
	while (p)
	{
		if (p->kind == NK_Function)
		{
			CheckFunction((AstFunction)p);
		}
		else
		{
			assert(p->kind == NK_Declaration);
			CheckGlobalDeclaration((AstDeclaration)p);
		}
		p = p->next;
	}
	CheckIncompleteGlobals();	
}


