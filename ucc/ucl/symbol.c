#include "ucl.h"
#include "output.h"



typedef struct  bucketLinker{
	struct bucketLinker * link;
	Symbol sym;
} * BucketLinker;



// number of strings
static int StringNum;
/*********************************************************
	Three global hashtables.
 *********************************************************/
// tags in global scope, tag means struct/union, enumeration name
static struct table GlobalTags;
// normal identifiers in global scope
static struct table GlobalIDs;
// all the constants
static struct table Constants;
// tags in current scope
static Table Tags;
// normal identifiers in current scope
static Table Identifiers;

// see examples/scope/parameterList.c
static int inParameterList = 0;
static Table savedIdentifiers,savedTags;

int IsInParameterList(void){
	return inParameterList;
}

void EnterParameterList(void){
	inParameterList = 1;
	EnterScope();	
}
void LeaveParemeterList(void){
	inParameterList = 0;
	ExitScope();
}
void SaveParameterListTable(void){
	savedIdentifiers = Identifiers;
	savedTags = Tags;
}
void RestoreParameterListTable(void){
	Level++;
	savedIdentifiers->outer = Identifiers;
	savedIdentifiers->level = Level;
	Identifiers = savedIdentifiers;
		
	savedTags->outer = Tags;
	savedTags->level = Level;		
	Tags = savedTags;
}

/*************************************************************
	Four Linked-list
 *************************************************************/

// used to construct symobl list
static Symbol *FunctionTail, *GlobalTail, *StringTail, *FloatTail;
// all the function symbols
Symbol Functions;
// all the gloabl variables and static variables
Symbol Globals;
// all the strings
// see EmitStrings(void)
Symbol Strings;
// all the floating constants
Symbol FloatConstants;
/// Scope level, file scope will be 0, when entering each nesting level,
/// Level increment; exiting each nesting level, Level decrement
int Level;
// number of temporaries
int TempNum;
// number of labels, see CreateLabel(void)
int LabelNum;

/**
 * Look up 'name' in current scope and all the enclosing scopes
 */
#define	SEARCH_OUTER_TABLE	1

static const char * symKindName[] = {
	"SK_Tag",    "SK_TypedefName", "SK_EnumConstant", "SK_Constant", "SK_Variable", "SK_Temp",
	"SK_Offset", "SK_String",      "SK_Label",        "SK_Function", "SK_Register",
	"SK_IRegister","SK_NotAvailable"
};
static Symbol DoLookupSymbol(Table tbl, char *name, int  searchOuter){
	Symbol p;
	/*********************************************
		h is used as key to search the hashtable.
	 *********************************************/
	unsigned h = (unsigned long)name & SYM_HASH_MASK;
	BucketLinker linker;
	do{
		if (tbl->buckets != NULL){
			for (linker =(BucketLinker) tbl->buckets[h]; linker; linker = linker->link)	{
				if (linker->sym->name == name){
					linker->sym->level = tbl->level;
					return  linker->sym;
				}
			}
		}
	} while ((tbl = tbl->outer) != NULL && searchOuter);
	return NULL;	
}
static Symbol LookupSymbol(Table tbl, char *name)
{
	return DoLookupSymbol(tbl,name,SEARCH_OUTER_TABLE);
}
const char * GetSymbolKind(int kind){
	return symKindName[kind];
}



/**
 * Add a symbol sym to symbol table tbl
 */
static Symbol AddSymbol(Table tbl, Symbol sym)
{
	unsigned int h = (unsigned long)sym->name & SYM_HASH_MASK;
	BucketLinker  linker;
	CALLOC(linker);
	if (tbl->buckets == NULL)
	{
		int size = sizeof(Symbol) * (SYM_HASH_MASK + 1);

		tbl->buckets = HeapAllocate(CurrentHeap, size);
		memset(tbl->buckets, 0, size);
	}
	// add the new symbol into the first positon of bucket[h] list.
	linker->link = (BucketLinker) tbl->buckets[h];
	linker->sym = sym;
	sym->level = tbl->level;
	tbl->buckets[h] = (Symbol) linker;;
	return sym;
}

/**
 * Enter a nesting scope. Increment the nesting level and 
 * create two new symbol table for normal identifiers and tags.
 */
void EnterScope(void)
{
	Table t;

	Level++;

	ALLOC(t);
	t->level = Level;
	t->outer = Identifiers;
	t->buckets = NULL;
	Identifiers = t;

	ALLOC(t);
	t->level = Level;
	t->outer = Tags;
	t->buckets = NULL;
	Tags = t;
}

/**
 * Exit a nesting scope. Decrement the nesting level and 
 * up to the enclosing normal identifiers and tags
 */
void ExitScope(void)
{
	Level--;
	Identifiers = Identifiers->outer;
	Tags = Tags->outer;
}

Symbol LookupID(char *name)
{
	return LookupSymbol(Identifiers, name);
}

Symbol LookupTag(char *name)
{
	return LookupSymbol(Tags, name);
}
// Tag:  struct/union/enum name
/******************************************
	There may be two symbols for "Data" in hashtable Tags,
	but with different levels. ?
	
	struct Data{
		//...
	};
	void f(){
		struct Data{
			//	...
		};
		
	}

	struct A{
		struct B{
		}
	};
			AddTag A, level = 0
			AddTag B, level = 0
******************************************/

Symbol AddTag(char *name, Type ty, Coord pcoord)
{
	Symbol p;

	CALLOC(p);
	// PRINT_DEBUG_INFO(("AddTag %s, level = %d",name, Level));
	p->kind = SK_Tag;
	p->name = name;
	p->ty = ty;
	p->pcoord = pcoord;
	//  see examples/scope/parameterList.c
	if(IsInParameterList()){		
		Warning(pcoord, "declaration of '%s %s' will not be visible outside of this function",
			GetCategName(ty->categ), name ?name:"<anonymous>");
	}

	return AddSymbol(Tags, p);
}

Symbol AddEnumConstant(char *name, Type ty, int val, Coord pcoord)
{
	Symbol p;

	CALLOC(p);
	p->pcoord = pcoord;
	p->kind = SK_EnumConstant;
	p->name = name;
	p->ty   = ty;
	p->val.i[0] = val;

	return AddSymbol(Identifiers, p);
}
/********************************************************
 typedef struct {
	 char *Istack[20];
 } block_debug ;	----------------- This is not treated as Tag,
 So we use AddTypedefName ,not AddTag().
 ********************************************************/
Symbol AddTypedefName(char *name, Type ty,Coord pcoord)
{
	Symbol p;

	CALLOC(p);
	// PRINT_DEBUG_INFO(("AddTypedefName %s, level = %d",name, Level));
	p->kind = SK_TypedefName;
	p->name = name;
	p->ty = ty;
	p->pcoord = pcoord;
	return AddSymbol(Identifiers, p);
}

Symbol AddVariable(char *name, Type ty, int sclass,Coord pcoord)
{
	VariableSymbol p;

	CALLOC(p);

	p->kind = SK_Variable;
	p->name = name;
	p->ty = ty;
	p->sclass = sclass;
	p->pcoord = pcoord;
	if (Level == 0 || sclass == TK_STATIC)
	{
		*GlobalTail = (Symbol)p;
		GlobalTail = &p->next;
	}
	else if (sclass != TK_EXTERN)
	{
		*FSYM->lastv = (Symbol)p;
		FSYM->lastv = &p->next;
	}
	if(sclass == TK_EXTERN  && Identifiers  != &GlobalIDs){
		AddSymbol(&GlobalIDs, (Symbol)p);		
	}
	return AddSymbol(Identifiers, (Symbol)p);
}


Symbol AddFunction(char *name, Type ty, int sclass,Coord pcoord)
{

	FunctionSymbol p;

	CALLOC(p);
	//PRINT_DEBUG_INFO(("%s at %s:%d",name,coord->filename,coord->ppline));
	p->kind = SK_Function;
	p->name = name;
	p->ty = ty;
	p->sclass = sclass;
	p->lastv = &p->params;
	p->pcoord = pcoord;
	
	*FunctionTail = (Symbol)p;
	FunctionTail = &p->next;

	if(Identifiers  != &GlobalIDs){
		//PRINT_DEBUG_INFO(("%s at %s:%d",name,pcoord->filename,pcoord->ppline));
		AddSymbol(Identifiers, (Symbol)p);		
	}

	return AddSymbol(&GlobalIDs, (Symbol)p);


}
/********************************************
	Lookup a const first, 
	if not existing, then add a new one.

	Constant:
			int, pointer, float,double
 ********************************************/
Symbol AddConstant(Type ty, union value val)
{
	unsigned h = (unsigned)val.i[0] & SYM_HASH_MASK;
	Symbol p;

	ty = Unqual(ty);
	if (IsIntegType(ty))
	{
		ty = T(INT);
	}
	else if (IsPtrType(ty))
	{
		ty = T(POINTER);
	}
	else if (ty->categ == LONGDOUBLE)
	{
		ty = T(DOUBLE);
	}
	/**************************************************
		If there is already a same const in hastable Constants,
			(type and value both are the same)
		just return it.
	 **************************************************/
	for (p = Constants.buckets[h]; p != NULL; p = p->link)
	{
		if (p->ty == ty && p->val.i[0] == val.i[0] && p->val.i[1] == val.i[1])
			return p;
	}
	// If not existing, we will create a new one.
	CALLOC(p);

	p->kind = SK_Constant;
	switch (ty->categ)
	{
	case INT:
		p->name = FormatName("%d", val.i[0]);
		break;

	case POINTER:	// name for POINTER const is 0x12345678, that is ,its address.
		if (val.i[0] == 0)
		{
			p->name = "0";
		}
		else
		{
			p->name = FormatName("0x%x", val.i[0]);
		}
		break;

	case FLOAT:
		p->name = FormatName("%g", val.f);
		break;

	case DOUBLE:
		p->name = FormatName("%g", val.d);
		break;

	default:
		assert(0);
	}

	p->pcoord = FSYM->pcoord;

	p->ty = ty;
	p->sclass = TK_STATIC;
	p->val = val;
	// insert the new const into hashtable bucket.
	p->link = Constants.buckets[h];
	Constants.buckets[h] = p;
	// if it is a float const, added to end of FloatConsts linked-list.
	if (ty->categ == FLOAT || ty->categ == DOUBLE)
	{
		*FloatTail = p;
		FloatTail = &p->next;
	}
	return p;
}

Symbol IntConstant(int i)
{
	union value val;

	val.i[0] = i;
	val.i[1] = 0;

	return AddConstant(T(INT), val);
}
/*********************************************
	 add string into linked-list Strings.
	 We don't check whether there is already a string of the same content.

	 ty is always not  WCharType, to be checked.  Bug ?
 *********************************************/
Symbol AddString(Type ty, String str,Coord pcoord)
{
	Symbol p;

	CALLOC(p);
	p->pcoord = pcoord;
	p->kind = SK_String;
	p->name = FormatName("str%d", StringNum++);
	p->ty = ty;
	p->sclass = TK_STATIC;
	p->val.p = str;

	*StringTail = p;
	StringTail = &p->next;

	return p;
}
/***************************************
	the temporay variable's name in a function is  t1,t2,....
 ***************************************/
Symbol CreateTemp(Type ty)
{
	VariableSymbol p;

	CALLOC(p);

	p->kind = SK_Temp;
	p->name = FormatName("t%d", TempNum++);
	p->ty = ty;
	p->level = 1;

	p->pcoord = FSYM->pcoord;

	*FSYM->lastv = (Symbol)p;
	FSYM->lastv = &p->next;

	return (Symbol)p;
}
// mainly create basic block's label name.
Symbol CreateLabel(void)
{
	Symbol p;

	CALLOC(p);

	p->kind = SK_Label;
	p->name = FormatName("BB%d", LabelNum++);

	p->pcoord = FSYM->pcoord;
	
	return p;
}

Symbol CreateOffset(Type ty, Symbol base, int coff,Coord pcoord)
{
	VariableSymbol p;
	//PRINT_DEBUG_INFO(("%s %d",base->name,coff));
	/********************************************************
	see examples/struct/FirstFieldType.c
		When coff is zero,
		the address of first field of struct and @base are same.
		But,
		their types are different.
	 ********************************************************/
	// 		for better uilasm.c, see TranslateCompoundStatement(AstStatement stmt)
	//		double d = 1.23;		---->		d[0] = 1.23;	--->	d = 1.23;
	if(coff == 0 && (IsArithType(base->ty) || (ty == base->ty))){
		return base;
	}

	CALLOC(p);
	if (base->kind == SK_Offset)
	{
		/*************************************************************
		struct Data{
			int e;	
			struct{
				int d;
				int c;
				int a;
			} i;
			int b[3];	
		};
		struct Data dt;
		int main(){
			dt.i.a = 30;		// we don't get here when compiling this statement.
			dt.b[1] = 20;		// we get here when compiling this statement.
			return 0;
		}
						symbol.c 640:  hello.c 16 :dt[16]
		 *************************************************************/
		coff += AsVar(base)->offset;
		base = base->link;
	}
	p->pcoord = pcoord;
	p->addressed = 1;
	p->kind = SK_Offset;
	p->ty = ty;
	/******************************************************
		For the use of link ,
			see "case SK_Offset" in function GetAccessName()
	 *******************************************************/
	p->link = base;
	p->offset = coff;	
	p->name = FormatName("%s[%d]", base->name, coff);
	base->ref++;

	return (Symbol)p;
}
/***********************************************
	This function is called when compiler compiles a new C file.
 ***********************************************/
void InitSymbolTable(void)
{
	int size;

	Level = 0;
	//	Hashtable	data-structure
	GlobalTags.buckets = GlobalIDs.buckets = NULL;
	GlobalTags.outer = GlobalIDs.outer = NULL;
	GlobalTags.level = GlobalIDs.level = 0;

	size = sizeof(Symbol) * (SYM_HASH_MASK + 1);
	Constants.buckets = HeapAllocate(CurrentHeap, size);
	memset(Constants.buckets, 0, size);
	Constants.outer = NULL;
	Constants.level = 0;
	//	Linked-list data-structure
	Functions = Globals = Strings = FloatConstants = NULL;
	FunctionTail = &Functions;
	GlobalTail = &Globals;
	StringTail = &Strings;
	FloatTail = &FloatConstants;

	Tags = &GlobalTags;
	Identifiers = &GlobalIDs;

	TempNum = LabelNum = StringNum = 0;
}

