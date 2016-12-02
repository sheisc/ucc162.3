#include "ucl.h"
#include "output.h"
#include "type.h"
#include "config.h"
// primary type: CHAR, ..., DOUBLE, ... , POINTER,VOID.  
// in fact, POINTER should be considered as a kind of type operator ?
struct type Types[VOID - CHAR + 1];
Type DefaultFunctionType;
Type WCharType;

static const char * categNames[] = {
	"CHAR", "UCHAR", "SHORT", "USHORT", "INT", "UINT", "LONG", "ULONG", "LONGLONG", "ULONGLONG", "ENUM",
	"FLOAT", "DOUBLE", "LONGDOUBLE", "POINTER", "VOID", "UNION", "STRUCT","ARRAY", "FUNCTION","NA"
};
const char * GetCategName(int categ){
	return categNames[categ];
}
/******************************************************************************
	typedef struct{
		int arr[4];
	}Data;
	
	void f(const Data ptr);
	int main(){
	
		return 0;
	}
	Because function Qualify() only copies part of object recordType/enumType/arrayType.
	So we have to unqualied it before calling the following function.
 ******************************************************************************/
//
int IsZeroSizeArray(Type ty){
	ty = Unqual(ty);
	return (ty->categ == ARRAY && (((ArrayType)ty)->len == 0) && ty->size == 0);	 
}
//
int IsIncompleteEnum(Type ty){
	ty = Unqual(ty);
	return ( ty->categ == ENUM  &&  !((EnumType)ty)->complete );
}
//
int IsIncompleteRecord(Type ty){
	ty = Unqual(ty);
	return ( IsRecordType(ty) && !((RecordType) ty)->complete);
}

// we  test whether it is incomplete enum/struct/union or array of these objects.
int IsIncompleteType(Type ty, int ignoreZeroArray){
	ty = Unqual(ty);
	switch(ty->categ){
	case ENUM:
		return IsIncompleteEnum(ty);
	case STRUCT:
	case UNION:
		return IsIncompleteRecord(ty);
	case ARRAY:
		if(ignoreZeroArray){
			return IsIncompleteType(ty->bty,IGNORE_ZERO_SIZE_ARRAY);
		}else{
			if(IsZeroSizeArray(ty)){
				return 1;
			}else{
				return IsIncompleteType(ty->bty,!IGNORE_ZERO_SIZE_ARRAY);
			}
		}
	default:
		return 0;
	}
}

/**
 * Apply default argument promotion to a type ty
 */
Type Promote(Type ty)
{
	/****************************************************************
		If the expression that denotes the called function has a type that
		does not include a prototype, the integral promotions are performed on
		each argument and arguments that have type float are promoted to double.  
		These are called the default argument promotions. 
		
		For example:

		
		#include <stdio.h>
		typedef int (*FUNC)();
		// C old-style declaration, without prototype.
		int f();
		int f(a,b);
		// new-style declaration, with prototype.
		//int f(int a,int b);
		
		//	We have both the old-style and the new-style declaration here.
		//	Then there will be a composite type of them.
		//	The compoiste type of 'int h()'  and  'int h(int a, int b)'
		//	shall be 'int h(int a, int b)' .
		//	
		//	If we comment the new-style declaration, // int h(int a, int b)
		//	the result is quite amazing.
		
		// without prototype
		int h();
		int h(a,b);
		// with prototype
		//int h(int a, int b);
		int k(){
			// when we are here,
			// the composite-type for f is "int f()" ,without prototype.
			// we do argument-promotion:	
			//	float 3.0f is promoted to double 3.0,	0x40080000 0x00000000
			//	float 2.0f is promoted to double 2.0. 
			printf("%x\n",f(3.0f,2.0f));	// 40080000
			// the composite-type for h is "int h()" ,without prototype
			printf("%x\n",h(3.0f,2.0f));	// 40080000 
			return 0;
		}
		// function defintion,
		// also acts a function-declaration, but without prototype.
		// when we are here, the composite type for 'f' is still "int f()", without prototype
		int f(a,b)
		int a,b;
		{
			return (a>b ? a : b);
		}
		// function definition,
		// also acts a function-declaration, with prototype.
		// when we are here, the prototype for h is "int h(int a,int b)",with prototype.
		int h(int a,int b){
			return (a>b ? a : b);
		}
		union{
			double fp64;
			int i_val[2];
		}u;
		int main(){
			FUNC fun;
		
			fun = k;
		
			(*fun)();
			u.fp64 = 3.0f;
			printf("%x %x\n",u.i_val[0],u.i_val[1]);
		
			printf("%x\n",f(3.0f,2.0f));	// 40080000
			// Here, the definition of h() 
			// acts as a function declaration with prototype.
			// The composite type for 'h' is "int h(int a,int b)".
			// 3.0f is converted to int 3, 2.0f is converted to int 2,
			// just like we call h((int)3.0f,(int)2.0f)
			printf("%x\n",h(3.0f,2.0f));	// 3 
			return 0;
		}



		From the above example, we can find that the sematics of the old-style declaration
		is absolutly different from the new-style.
		The result of the old one seems a little anti-intuition.
	 ****************************************************************/
	return ty->categ < INT ? T(INT) : (ty->categ == FLOAT ? T(DOUBLE) : ty);
}

/**
 * Check if two functions are compatible.
 * If compatible, return 1; otherwise, return 0
 */
static int IsCompatibleFunction(FunctionType fty1, FunctionType fty2)
{
	Signature sig1 = fty1->sig;
	Signature sig2 = fty2->sig;
	Parameter p1, p2;
	int parLen1, parLen2;
	int i;

	// return types shall be compatible
	/****************************************************************
		For two function types to be compatible,
		both shall specify compatible return types.
	 ****************************************************************/
	if (! IsCompatibleType(fty1->bty, fty2->bty))
		return 0;
	// if they both don't have prototype and do have Compatible return type, then they are compatible.
	if (! sig1->hasProto && ! sig2->hasProto)
	{
		return 1;
	}
	else if (sig1->hasProto && sig2->hasProto)
	{
		/*****************************************************************
			Moreover, the parameter type lists, if both are present, shall agree in the number of 
			parameters and in use of the ellipsis terminator; corresponding parameters shall 
			have compatible types.	
		 *****************************************************************/
		parLen1 = LEN(sig1->params);
		parLen2 = LEN(sig2->params);
		// If one has variable parameters while the other one not, 
		// or their parameter count are not the same, they are incompatible functions.
		/***********************************************************
		void f(int a,...){

		}
		void f(int a);	------------ incompatible in UCC
		
		int main(){

			f(1,2);
			
			return 0;
		}
		// It is an error in UCC, while a warning in VS2008.
		 ***********************************************************/
		if ((sig1->hasEllipsis ^ sig2->hasEllipsis) || (parLen1 != parLen2))
			return 0;
		// recursive check whether parameters are compatible.
		for (i = 0; i < parLen1; ++i)
		{
			p1 = (Parameter)GET_ITEM(sig1->params, i);
			p2 = (Parameter)GET_ITEM(sig2->params, i);

			if (! IsCompatibleType(p1->ty, p2->ty))
				return 0;
		}

		return 1;
	}
	else if (! sig1->hasProto && sig2->hasProto)
	{
		/************************************
			we  swap(sig1, sig2);
			then we can treat(! sig1->hasProto && sig2->hasProto)
			as (! sig2->hasProto && sig1->hasProto).
		 ************************************/
		sig1 = fty2->sig;
		sig2 = fty1->sig;
		goto mix_proto;
	}
	else
	{
mix_proto:
		/**************************************
		 ( sig1->hasProto && !sig2->hasProto):
			sig1 has Prototype and sig2 has not Prototype.
		 **************************************/

		parLen1 = LEN(sig1->params);
		parLen2 = LEN(sig2->params);
		/*****************************
		void f(int,...); --- sig1
		void f(); --- sig2
				----------------incompatible
				
		void f(a,b,c);	-------------					
		 		(hello.c,5):error:Identifier list should be in definition.

		void f(a,b,c) int a,b,c;{	--------------legal
		}
		 *****************************/	
		if (sig1->hasEllipsis)
			return 0;		
		if (parLen2 == 0)
		{
		/**********************************************************************
			If one type has a parameter type list and the other type
			is specified by a function declarator that is not part of a function definition and 
			that contains an empty identifier list, the parameter list shall not have an ellipsis
			terminator and the type of each parameter shall be compatible with the type that
			results from the application of the default argument promotions.  
			See function Promote() for the reason that we have to Promote(p1->ty) here.
			(1)
			sig1:
					f(int a,char b)
			sig2:
					f()
					--------------------	incompatible
			(1)
			sig1:
					f(int a,int b)
			sig2:
					f()
					--------------------	compatible								
		 ***********************************************************************/
			FOR_EACH_ITEM(Parameter, p1, sig1->params)
				if (! IsCompatibleType(Promote(p1->ty), p1->ty))
					return 0;
			ENDFOR

			return 1;
		}
		else if (parLen1 != parLen2)
		{
			/*************************************
			 	void f(int,int);
				void f(a,b,c) int a,b,c;{
				}
					------------------- incompatible
			 **************************************/
			return 0;
		}
		else
		{
			/**************************************************************************	
			If one type has a parameter type list and the other type is specified by a function
			definition that contains a (possibly empty) identifier list, both 
			shall agree in the number of parameters, and the type of each prototype parameter
			shall be compatible with the type that results from the application of the default 
			argument promotions to the type of the corresponding identifier.  
			(For each parameter declared with function or array type, its type for these comparisons
			is the one that results from conversion to a pointer type,
			as in $3.7.1.  For each parameter declared with qualified type, 
			its type for these comparisons is the unqualified version of its declared type.)	

			void f(int,int);
			void f(a,b) int a,b;{
			}	---------------------  compatible

			
			void f(int,char);
			void f(a,b) int a,b;{
			}	---------------------  incompatible			
			 ***************************************************************************/
			for (i = 0; i < parLen1; ++i)
			{
				p1 = (Parameter)GET_ITEM(sig1->params, i);
				p2 = (Parameter)GET_ITEM(sig2->params, i);
				/*********************************************
					Bug ?
					According to C89, we should compare the unqualified version?	
				 *********************************************/
				if (! IsCompatibleType(p1->ty, Promote(p2->ty)))
					return 0;
			}		
			return 1;
		}
	}				
}


/**
 * Check if two types are compatible
 * If compatible, return 1; otherwise, return 0
 */


/**********************************************

#include <stdio.h>
 
 
 struct Data{
	 int a;
 };
 void SetData(struct Data * d){
	 d->a = 0x12345;
 }
 int main(){
 
	 struct Data{
		 double a;
	 };
 
	 struct Data dt;
	 dt.a = 3.0;
	 printf("%x %x.\n",dt.a);
	 SetData(&dt);		-------------------->  VS2008:  a warning, InCompatible type.
	 printf("%x %x.\n",dt.a);
	 // double 3.0	 is 0x00000000 0x40080000	 LITTLE_ENDIAN
	 // 3.0f is prototed to double 3.0,the pushed into stack
	 printf("%x %x.\n",3.0f);
	 return 0;
 }

 **********************************************/
 
int IsCompatibleType(Type ty1, Type ty2)
{

	if (ty1 == ty2)
		return 1;
	/**************************************************
	// const / volatile qualifier
	For two qualified types to be compatible, both shall have the identically 
	qualified version of a compatible type; the order of type qualifiers within 
	a list of specifiers or qualifiers does not care.
	 ***************************************************/
	if (ty1->qual != ty2->qual)
		return 0;
	// throw away the const/volatile qualifier
	ty1 = Unqual(ty1);
	ty2 = Unqual(ty2);
	/**********************************************
	 see function Enum(char *id)
	 	ty1 is ENUM, ty2 is T(INT)
	 	or
	 	ty2 is ENUM, ty1 is T(INT)
	 	
	 	see <http://flash-gordon.me.uk/ansi.c.txt>
	 	Each enumerated type shall be compatible with an integer type; 
	 	the choice of type is implementation-defined.
	 **********************************************/
	#if 0 	// commented
	if (ty1->categ == ENUM && ty2 == ty1->bty ||
		ty2->categ == ENUM && ty1 == ty2->bty)
		return 1;
	#endif
	if (ty1->categ != ty2->categ)
		return 0;
	// we are sure now: ty1 and ty2 have same category.
	/*******************************************************
		What will happen when  categ is STRUCT/UNION ?
	 *******************************************************/
	switch (ty1->categ)
	{
	case POINTER:	// recursive check ty1->bty and ty2->bty
		/***************************************************
			For two pointer types to be compatible, both shall be identically 
			qualified and both shall be pointers to compatible types.
		 ****************************************************/
		return IsCompatibleType(ty1->bty, ty2->bty);

	case ARRAY:		
		/*********************************************************
			recursive check ty1->bty and ty2->bty;
			they have same size or one type has zero size.		
			int arr1[]; int arr2[3] ----> compatible
			int arr1[3]; int arr2[3] ----> compatible	

			For two array types to be compatible, both shall have compatible element types,
			and if both size specifiers are present, they shall have the same value.
		 *********************************************************/
		return IsCompatibleType(ty1->bty, ty2->bty) && 
		       (ty1->size == ty2->size || ty1->size == 0 || ty2->size == 0);

	case FUNCTION:
		return IsCompatibleFunction((FunctionType)ty1, (FunctionType)ty2);

	default:	// STRUCT/UNION
		return ty1 == ty2;
	}
}

/**
 * A very important principle: for those type constructing functions, they must keep
 * the integrity of the base type.
 */

/**
 * Construct an enumeration type whose name is id. id can be null
 */
Type Enum(char *id)
{
	EnumType ety;

	ALLOC(ety);

	ety->categ = ENUM;
	ety->id = id;

	// enumeration type is compatilbe with int
	ety->bty = T(INT);
	ety->size = ety->bty->size;
	ety->align = ety->bty->align;
	ety->qual = 0;

	return (Type)ety;
}

//  a little ugly, just for safe.  see examples/qualfier/qualify.c
static Type DoTypeClone(Type ty){
	int categ = ty->categ;
	if(categ == STRUCT || categ == UNION){
		RecordType rty;
		CALLOC(rty);
		*rty = *((RecordType)ty);
		return (Type) rty;
	}else if(categ == ARRAY){
		ArrayType aty;
		CALLOC(aty);
		*aty = *((ArrayType)ty);
		return (Type) aty;
	}else if(categ == FUNCTION){
		FunctionType fty;
		CALLOC(fty);
		*fty = *((FunctionType)ty);
		return (Type) fty;	
	}else if(categ == ENUM){
		EnumType ety;
		CALLOC(ety);
		*ety = *((EnumType)ty);
		return (Type) ety;	
	}else{
		Type qty;
		CALLOC(qty);
		*qty = *ty;
		return qty;
	}

}

/**
 * Contruct a qualified type
 */
Type Qualify(int qual, Type ty)
{

	/**********************************************
		see examples/qulifier/qualify.c
	 **********************************************/
	Type qty;
	// @qual is const or volatile
	// if ty has already been qualified by @qual or qual is zero, just return @ty itself.
	if (qual == 0 || qual == ty->qual)
		return ty;
	// we don't want to change the @ty. So we copy @ty first.
	qty = DoTypeClone(ty);
	qty->qual |= qual;
	// if ty has been qualified, get the original type(not qualified one)
	// else ty has not been qualified, ty is the original type.
	if (ty->qual != 0)
	{
		qty->bty = ty->bty;
	}
	else
	{
		qty->bty = ty;
	}
	return qty;	

}

/**
 * Get a type's unqualified version
 */
Type Unqual(Type ty)
{
	if (ty->qual)
		ty = ty->bty;
	return ty;
}

/**
 * Construct an array type, len is the array length.
 */
Type ArrayOf(int len, Type ty)
{

	ArrayType aty;

	CALLOC(aty);

	aty->categ = ARRAY;
	aty->qual = 0;
	aty->size = len * ty->size;
	aty->align = ty->align;
	aty->bty = ty;
	aty->len = len;
	return (Type)aty;	

}

/**
 * Construct a type pointing to ty.
 */
Type PointerTo(Type ty)
{
	Type pty;

	ALLOC(pty);

	pty->categ  = POINTER;
	pty->qual = 0;
	pty->align = T(POINTER)->align;
	pty->size = T(POINTER)->size;
	pty->bty = ty;

	return pty;
}

/**
 * Construct a function type, the return type is ty. 
 */
Type FunctionReturn(Type ty, Signature sig)
{
	FunctionType fty;

	ALLOC(fty);

	fty->categ = FUNCTION;
	fty->qual = 0;
	fty->size = T(POINTER)->size;
	fty->align = T(POINTER)->align;
	fty->sig = sig;
	fty->bty = ty;

	return (Type)fty;
}

/**
 * Construct a struct/union type whose name is id, id can be NULL.
 */
Type StartRecord(char *id, int categ)
{
	// @categ is STRUCT or UNION;
	RecordType rty;

	ALLOC(rty);
	memset(rty, 0, sizeof(*rty));
	/*************************************
		When Starting parsing struct /union,
		its size is unclear yet.
		So rty->size is 0 when calling this function.
	 *************************************/
	rty->categ = categ;
	rty->id = id;
	rty->tail= &rty->flds;

	rty->complete = 0;

	return (Type)rty;
}

/**
 * Add a field to struct/union type ty. fty is the type of the field.
 * id is the name of the field. If the field is a bit-field, bits is its
 * bit width, for non bit-field, bits will be 0.
 */
/*******************************************************
struct Data{
	int a:2;		------------>   Field
	......
}
	@ty		is a object of RecordType, the type of "Data"
	@id		field name,	"a"
	@fty	the type of struct field	, "int"
	@bits	whether this field is bit field, if yes, @bits is the length of bits, "2"
 *******************************************************/
Field AddField(Type ty, char *id, Type fty, int bits)
{
	RecordType rty = (RecordType)ty;
	Field fld;

	if (fty->size == 0)		// int arr[];
	{
		if(fty->categ == ARRAY){
			rty->hasFlexArray = 1;	
		}
	}
	// If one field member is const ,the total struct object is const.
	if (fty->qual & CONST)
	{
		rty->hasConstFld = 1;
	}

	ALLOC(fld);

	fld->id = id;
	fld->ty = fty;
	fld->bits = bits;
	// offset will be determined in function EndRecord(Type ty)
	fld->pos = fld->offset = 0;
	fld->next = NULL;

	*rty->tail = fld;
	rty->tail  = &(fld->next);

	return fld;
}

/**
 * Lookup if there is a field named id in struct/union type
 */
Field LookupField(Type ty, char *id)
{
	RecordType rty = (RecordType)ty;
	Field fld = rty->flds;

	while (fld != NULL)
	{
		// unnamed struct/union field in a struct/union
		if (fld->id == NULL && IsRecordType(fld->ty))
		{
			/**************************************
				(1)
				typedef struct Data{
					int a;
					struct{
						int b;			// Dada d;  d.b = 5;
					};	
				}Data;
				(2)
				typedef struct Data{
					int a;
					struct{
						int a;	// OK.		Data d;  d.b.a = 3;
					} b;	// this is not an unnamed filed. its name is b. Though the struct is unnamed.	
				}Data;
				(3)
				typedef struct Data{
					int a;
					struct{
						int a;	// redefinition of a
					} ;	
				}Data;				
			 **************************************/
			Field p;

			p = LookupField(fld->ty, id);
			if (p)
				return p;
		}
		else if (fld->id == id)	// see function  InternName() and AppendSTR()
			return fld;

		fld = fld->next;
	}

	return NULL;
}

/**
 * For unamed struct/union field in a struct/union, the offset of fields in the 
 * unnamed struct/union should be fix up.
 */
/******************************************
	struct Data{
		int a;		-----	offset is 0
		struct{
			int b1;	-----0
			int b2;	-----4
			int b3;	-----8		
		};			-----	offset is  4
		double c;	-----	offset is 16
		struct{
			int d1;	----	0
			int d2;	----	4
			int d3;	----	8
		}d;					//  
	}
	When we know the offset for 'b' is 4,
	we call AddOffset(...) to calculate the offset of 
	b1,b2,b3 .
		AddOffset(..,  4)
			b1:		offset	0+4		4
			b2:		offset    4+4		8
			b3:		offset	4+8		12
		But for d1,d2,d3, we don't call AddOffset(..).
	For example:
		Data data;
		data.d.d1;	----------      data.d ------ knowing the offset of d in is enough.
					---------- 	d.d1    ------ knowing the offset of d1 in data is also enough.
		data.b1;	----------	we must know the offset of b1 in data.
 ******************************************/
void AddOffset(RecordType rty, int offset)
{
	Field fld = rty->flds;

	//PRINT_DEBUG_INFO(("fld->id == NULL && IsRecordType(fld->ty)"));
	while (fld)
	{
		fld->offset += offset;
		if (fld->id == NULL && IsRecordType(fld->ty))
		{			
			AddOffset((RecordType)fld->ty, fld->offset);
		}
		fld = fld->next;
	}

}

/**
 * When a struct/union type's delcaration is finished, layout the struct/union.
 * Calculate each field's offset from the beginning of struct/union and the size
 * and alignment of struct/union.
 */
void EndRecord(Type ty, Coord coord)
{
	RecordType rty = (RecordType)ty;
	Field fld = rty->flds;
	int bits = 0;
	int intBits = T(INT)->size * 8;
	if (rty->categ == STRUCT)
	{
		/***************************************************
			At first, rty->size is 0.  
					rty->align is 0.
				See function StartRecord().
		 ***************************************************/
		while (fld)
		{
			
			// align each field
			fld->offset = rty->size = ALIGN(rty->size, fld->ty->align);			
			/***************************************************************
			when we call  recursive CheckStructOrUnionSpecifier(...):
				//  CheckStructOrUnionSpecifier(...)-------------------------(c1)
				
				typedef struct{ ------------------------StartRecord()	-----(s1)
					int a;
					
					// CheckStructOrUnionSpecifier(...)	----------------------(c2)
					struct{---------------- StartRecord()					-----(s2)
						int b;
					};		// fld->id is NULL.
						-------------------EndRecord()					----(e2)
						
				}Data; -------------------------------EndRecord()	-----(e1)

			calling sequence:
					(c1)	--->  (c2)
			ending sequence:
					(c2)	--->   (c1)
			In (c2), we call (s2) and (e2).
					we calculate the layout for current struct by calling (e2),  the inner struct shown above.
			In (c1), we call (s1) and (e1)		
					we calculate the layout for current struct by calling (e2),  the outer struct shown above.
			 *****************************************************************/
			if (fld->id == NULL && IsRecordType(fld->ty))
			{
				AddOffset((RecordType)fld->ty, fld->offset);
			}
			// process bit-field
			if (fld->bits == 0)
			{
				/// if current field is not a bit-field, whenever last field is bit-field or not, 
				/// it will occupy a chunk of memory of its size.
				if (bits != 0)
				{
					/**********************************************************
						struct {
							int a:12;
							//rty->size
							int b;	---------- we are here.
						}
					 **********************************************************/
					fld->offset = rty->size = ALIGN(rty->size + T(INT)->size, fld->ty->align);
				}
				bits = 0;
				rty->size = rty->size + fld->ty->size;
			}
			else if (bits + fld->bits <= intBits)
			{
				/******************************************
					struct {
						int a:12;
						...
					}
					or
					struct {
						int a:12;
						int b:12;
						....
					}
				 *******************************************/
				// current bit-field and previous bit-fields can be placed together into an int.
				// calculate the position in an 'int' for the current bit-field.
				fld->pos = LITTLE_ENDIAN ? bits : intBits - bits;
				bits = bits + fld->bits;
				// the int space for bit-fields are full now.
				if (bits == intBits)
				{
					rty->size += T(INT)->size;
					bits = 0;
				}
				//PRINT_DEBUG_INFO((""));
			}
			else
			{
				/// current bit-field can't be placed together with previous bit-fields,
				/// must start a new chunk of memory
				/*******************************************
					struct{
						int a:24;	---------------	a's offset is 0
						// rty->size is 0
						// rty->align is 4
						int b:30;
					}
				 *******************************************/
				rty->size += T(INT)->size;
				fld->offset += T(INT)->size;
				fld->pos = LITTLE_ENDIAN ? 0 : intBits - fld->bits;
				bits = fld->bits;
				/*******************************************
					struct{
						int a:24;	---------------	a's offset is 0
						// rty->size is 0
						// rty->align is 4
						int b:30;	---------------	b's offset is 4
						right now				
						// rty->size increments by T(INT)->size:		---  4
									the last filed 'a' will occupy the whole int space.
						// b's offset is:		--------  4
									the new value of rty->size
									that is the sum of the  old value of rty->size 
									(also the old value of field->offset)
									and T(int)->size.
									
					}
				 *******************************************/	
				 //PRINT_DEBUG_INFO((""));
			}
			/***********************************
				The align of struct is changing during calling 
				EndRecord()
			 ***********************************/
			if (fld->ty->align > rty->align)
			{
				rty->align = fld->ty->align;
			}
			fld = fld->next;
		}
		// the last field of struct is bit-filed 
		if (bits != 0)
		{
			rty->size += T(INT)->size;
		}
		rty->size = ALIGN(rty->size, rty->align);
	}
	else		//UNION
	{
		/*********************************************
		(1)
		The largest align of field member is the align of union.
		(2)
		The largest size of field member is the size of union.
		 *********************************************/
		while (fld)
		{
			// if the field member of a union has larger align 
			if (fld->ty->align > rty->align)
			{
				rty->align = fld->ty->align;
			}
			// // if the field member of a union has larger size 
			if (fld->ty->size > rty->size)
			{
				rty->size = fld->ty->size;
			}
			fld = fld->next;
		}
		//  for better diagnosis
		if(rty->hasFlexArray){
			Error(coord,"flexible array member in union");
		}
	}
	if(rty->categ == STRUCT && rty->size == 0 && rty->hasFlexArray){
		Error(coord,"flexible array member in otherwise empty struct");
	}
}

/**
 * Return the composite type of ty1 and ty2.
 */
 /*********************************************************
	see 
	<Rationale for International Standard Programming Languages C>
	
	The concepts of compatible type and composite type were introduced to
	allow C89 to discuss those situations in which type declarations
	need not be identical.
	Given the following two file scope declarations:
		int f(int (*)(), double (*)[3]);
		int f(int (*)(char *), double (*)[]);
	The resulting composite type for the function is:
		int f(int (*)(char *), double (*)[3]);

	From the view of situations a type declaration can be applied, 
	Composite type is like greatest common divisor.  GCD

See comments of function Promote()	above
	@ty1	one type
	@ty2	another type
	@return		the return type is compatible with @ty1 and @ty2
  **********************************************************/
Type CompositeType(Type ty1, Type ty2)
{
	// ty1 and ty2 must be compatible when calling this function
	/********************************************************
		 A composite type can be constructed from two types that are compatible;
		 it is a type that is compatible with both of the two types and has the following additions:
	 ********************************************************/
	assert(IsCompatibleType(ty1, ty2));
	/*************************************************
	 The gcd of INT and ENUM is ENUM?
	 Any situation a int can be used, a ENUM too.
	 But not vice versa ?
	 *************************************************/
	if (ty1->categ == ENUM)
		return ty1;

	if (ty2->categ == ENUM)
		return ty2;

	switch (ty1->categ)
	{
	case POINTER:
		/*************************************************************
			If ty1 and ty2 are compatible, their 'qualifier' are the same.
		 *************************************************************/
		return Qualify(ty1->qual, PointerTo(CompositeType(ty1->bty, ty2->bty)));

	case ARRAY:
		/****************************************************************
			 If one type is an array of known constant size, the composite type is 
			 an array of that size; otherwise, if one type is a variable length array,
			 the composite type is that type.
		 ****************************************************************/
		return ty1->size != 0 ? ty1 : ty2;

	case FUNCTION:
		{
			FunctionType fty1 = (FunctionType)ty1;
			FunctionType fty2 = (FunctionType)ty2;

			fty1->bty = CompositeType(fty1->bty, fty2->bty);
			/**********************************************************
				 If both types are function types with parameter type lists,
				 the type of each parameter in the composite parameter type list 
				 is the composite type of the corresponding parameters.	
			 **********************************************************/
			if (fty1->sig->hasProto && fty2->sig->hasProto)
			{
				Parameter p1, p2;
				int i, len = LEN(fty1->sig->params);

				for (i = 0; i < len; ++i)
				{
					p1 = (Parameter)GET_ITEM(fty1->sig->params, i);
					p2 = (Parameter)GET_ITEM(fty2->sig->params, i);
					p1->ty = CompositeType(p1->ty, p2->ty);
				}
				/**********************************************
					In fact , the original @ty1 has been modified here,
					why not create a New struct functionType here?
					Bug?
				 **********************************************/
				return ty1;
			}
			/*************************************************
				If only one type is a function type with a parameter type list
				(a function prototype),
				the composite type is a function prototype with the parameter type list.
			 *************************************************/
			return fty1->sig->hasProto ? ty1 : ty2;
		}

	default:
		return ty1;
	}
}

/**
 * Return the common real type for ty1 and ty2
 */
// from the view of values a type can represent, common real type is like least common multiple.LCM 
//  real type:	the type of real number	f
Type CommonRealType(Type ty1, Type ty2)
{
	/**********************************************************
		3.2.1.5 Usual arithmetic conversions   
		Many binary operators that expect operands of arithmetic type cause
		conversions and yield result types in a similar way.	
		The purpose is to yield a common type, 
		which is also the type of the result.  

		This pattern is called the usual arithmetic conversions: 
		First, if either operand has type long double, the other operand is converted to longdouble .
		Otherwise, if either operand has type double, the other operand is converted to double. 
		Otherwise, if either operand has type float, the other operand is converted to float.  
		
		Otherwise, the integral promotions are performed on both operands.	
		Then the following rules are applied: 
		If either operand has type unsigned long int, the other operand is converted to unsigned long int.

		Otherwise, if one operand has type long int and the other has type unsigned int,
		if a long int can represent all values of an unsigned int,
		the operand of type unsigned int is converted to long int ;
		if a long int cannot represent all the values of an unsigned int, 
		both operands are converted to unsigned long int.  

		Otherwise, if either operand has type long int, the other operand is converted to long int.
		Otherwise, if either operand has type unsigned int, the other operand is converted to unsigned int.  
		Otherwise, both operands have type int.
	 **********************************************************/
	if (ty1->categ == LONGDOUBLE || ty2->categ == LONGDOUBLE)
		return T(LONGDOUBLE);

	if (ty1->categ == DOUBLE || ty2->categ == DOUBLE)
		return T(DOUBLE);

	if (ty1->categ == FLOAT || ty2->categ == FLOAT)
		return T(FLOAT);
	// neither ty1 nor ty2 is floating number.
	ty1 = ty1->categ < INT ? T(INT) : ty1;
	ty2 = ty2->categ < INT ? T(INT) : ty2;

	if (ty1->categ == ty2->categ)
		return ty1;
	// ty1 and ty2 have the same sign
	if ((IsUnsigned(ty1) ^ IsUnsigned(ty2)) == 0)
		return ty1->categ > ty2->categ ? ty1 : ty2;
	// Their signs are different.
	// Swap ty1 and ty2, then we treat ty1 as Unsigned, ty2 as signed later.
	if (IsUnsigned(ty2))
	{
		Type ty;

		ty = ty1;
		ty1 = ty2;
		ty2 = ty;
	}
	// fg: (ty1,ty2) : ( ULONG,INT)
	if (ty1->categ  >= ty2->categ)
		return ty1;
	// fg: (ty1,ty2) : ( UINT, LONG)
	/*********************************
		if the size of UINT and LONG are both 4 bytes,
			we will return ULONG as the common real type later.
		If signed long is large enough to accommodate UINT,
			return LONG.
	 *********************************/
	if (ty2->size > ty1->size)
		return ty2;

	return T(ty2->categ + 1);

}

/**
 * Adjust paramter type
 */
Type AdjustParameter(Type ty)
{
	ty = Unqual(ty);
	/******************************************************
	Array expressions and function designators as arguments are converted 
	to pointers before the call.  A declaration of a parameter as ``array of type '' 
	shall be adjusted to``pointer to type ,'' and a declaration of a parameter as
	``function returning type '' shall be adjusted to
	``pointer to function returning type ,'	
	 ******************************************************/
	// int f(int arr[3]) ---->  int f(int *);
	if (ty->categ == ARRAY)
		return PointerTo(ty->bty);
	/***************************************************
		 A function designator is an expression that has function type.
		 Except when it is the operand of the sizeof operator or the unary& operator, 
		 a function designator with type ``function returning type'' is converted to 
		 an expression that has type ``pointer to function returning type .''
	 ***************************************************/
	if (ty->categ == FUNCTION)
		return PointerTo(ty);

	return ty;
}

/**
 * Although int and long are different types from C language, but from some underlying implementations,
 * the size and representation of them maybe identical. Actually, the function should be provided by 
 * the target. UCC defines a series of type code: 
 * I1: signed 1 byte       U1: unsigned 1 byte
 * I2: signed 2 byte       U2: unsigned 2 byte
 * I4: signed 4 byte       U4: unsigned 4 byte
 * F4: 4 byte floating     F8: 8 byte floating
 * V: no type              B: memory block, used for struct/union and array.
 * The target should provide TypeCode to define the map from type category to type code. And UCC can
 * add more type codes on demand of different targets.
 */
int TypeCode(Type ty)
{
	/***************************************************************************
		see type.h
		enum
		{
			CHAR, UCHAR, SHORT, USHORT, INT, UINT, LONG, ULONG, LONGLONG, ULONGLONG, ENUM,
			FLOAT, DOUBLE, LONGDOUBLE, POINTER, VOID, UNION, STRUCT, ARRAY, FUNCTION
		};
	 ***************************************************************************/
	static int optypes[] = {I1, U1, I2, U2, I4, U4, I4, U4, I4, U4, I4,/* float */ F4, F8, F8, U4, V, B, B, B};

	assert(ty->categ != FUNCTION);
	// Mapping CHAR ...  to  I1 ....
	return optypes[ty->categ];
}

/**
 * Get the type's user-readable string.
 */
char* TypeToString(Type ty)
{
	int qual;
	char *str;
	static char *names[] = 	{
		"char", "unsigned char", "short", "unsigned short", "int", "unsigned int",
		"long", "unsigned long", "long long", "unsigned long long", "enum", "float", "double",
		"long double"
	};	
	if (ty->qual != 0)
	{
		qual = ty->qual;
		ty = Unqual(ty);

		if (qual == CONST)
			str = "const";
		else if (qual == VOLATILE)
			str = "volatile";
		else 
			str = "const volatile";
		// for example: const volatile int
		return FormatName("%s %s", str, TypeToString(ty));
	}
	// primary type
	if (ty->categ >= CHAR && ty->categ <= LONGDOUBLE && ty->categ != ENUM)
		return names[ty->categ];

	switch (ty->categ)
	{
	case ENUM:
		return FormatName("enum %s", ((EnumType)ty)->id);

	case POINTER:
		return FormatName("%s *", TypeToString(ty->bty));

	case UNION:
		return FormatName("union %s", ((RecordType)ty)->id);

	case STRUCT:
		return FormatName("struct %s", ((RecordType)ty)->id);

	case ARRAY:
		return FormatName("%s[%d]", TypeToString(ty->bty), ty->size / ty->bty->size);

	case VOID:
		return "void";

	case FUNCTION:
		{
			FunctionType fty = (FunctionType)ty;
			// ignore the parameters ?
			return FormatName("%s ()", TypeToString(fty->bty));
		}

	default:
		assert(0);
		return NULL;
	}
}

/**
 * Setup the type system according to the target configuration.
 */
void SetupTypeSystem(void)
{
	int i;
	FunctionType fty;
	/************************************************
	// setup type size for each type.
		some elements in Types[] are just place-holders.
		e.g.  ENUM,  ?
	 *************************************************/
	T(CHAR)->size = T(UCHAR)->size = CHAR_SIZE;
	T(SHORT)->size = T(USHORT)->size = SHORT_SIZE;
	T(INT)->size = T(UINT)->size = INT_SIZE;
	T(LONG)->size = T(ULONG)->size = LONG_SIZE;
	T(LONGLONG)->size = T(ULONGLONG)->size = LONG_LONG_SIZE;
	T(FLOAT)->size = FLOAT_SIZE;
	T(DOUBLE)->size = DOUBLE_SIZE;
	T(LONGDOUBLE)->size = LONG_DOUBLE_SIZE;
	T(POINTER)->size = INT_SIZE;
	// without this, TypeToString() would have Segmentation fault
	T(POINTER)->bty = T(INT);

	// type category, type alignment
	for (i = CHAR; i <= VOID; ++i)
	{
		T(i)->categ = i;
		T(i)->align = T(i)->size;
	}
	/**************************************
		Construct a default function type:
			int f();
	 **************************************/
	ALLOC(fty);
	fty->categ = FUNCTION;
	fty->qual = 0;
	fty->align = fty->size = T(POINTER)->size;
	// for function type, its bty field is the type of the function return value.
	fty->bty = T(INT);
	// withou any parameters and no prototype, old-style declaration.
	ALLOC(fty->sig);
	CALLOC(fty->sig->params);
	fty->sig->hasProto = 0;
	fty->sig->hasEllipsis = 0;

	DefaultFunctionType = (Type)fty;
	WCharType = T(WCHAR);
}

