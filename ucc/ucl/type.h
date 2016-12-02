#ifndef __TYPE_H_
#define __TYPE_H_
/***************************************************************
// type category
// CHAR / UCHAR ... from the view of C language
// while I1/U1 ..  from the view of machine
(1)
	Because ENUM could be seen as a INT, so it is put before FLOAT.
(2)
	POINTER is considered as a scalar, not vector-type as struct.
 ***************************************************************/
enum
{
	CHAR, UCHAR, SHORT, USHORT, INT, UINT, LONG, ULONG, LONGLONG, ULONGLONG, ENUM,
	FLOAT, DOUBLE, LONGDOUBLE, POINTER, VOID, UNION, STRUCT, ARRAY, FUNCTION
};
// type qualifier
enum { CONST = 0x1, VOLATILE = 0x2 };
// I1 : signed int8	1byte=8bit;  U1: unsigned int8
// V: no type              B: memory block, 
// see TypeCode()
enum {I1, U1, I2, U2, I4, U4, F4, F8, V, B};
/***********************************************
	categ:	category	CHAR,UCHAR,SHORT, ...
	qual:	type qualifier, CONST, VOLATILE
	align:	type alignment
	size:	type size
	bty:		for primary type		bty  is NULL
			for derived type		not NULL,  fg, POINTER, FUNCTION,...
								(1)const int is treated as derived type ?
								(qual != 0 && bty == T(INT)) ?
								(2) ENUM,  see function Enum(char *id)
			bty stands for base type ?								
 ***********************************************/
#define TYPE_COMMON \
    int categ : 8;  \
    int qual  : 8;  \
    int align : 16; \
    int size;       \
    struct type *bty;
	
typedef struct type
{
	TYPE_COMMON
} *Type;


typedef struct arrayType{
	TYPE_COMMON
	int len;		// count of array elements
} * ArrayType;

/***************************************
	the field member of a struct or union
	offset:	offset to the beginning of struct/union
	id:		filed name
	bits:	bits of a bit filed; // int align:16;
			for non-bit files, bits is 0. //int size;
	pos:	see EndRecord() funcion: fld->pos = LITTLE_ENDIAN ? bits : intBits - bits;			
	ty:		field type
	next:	pointer to next field
 ***************************************/
typedef struct field
{
	int offset;
	char *id;
	int bits;
	int pos;
	Type ty;
	struct field *next;
} *Field;
/*********************************************************
	id:	struct/union name, for anonymous struct/union, id is NULL.
	flds: all the fields of struct/union
	tail:		used for construction of field list
	hasConstFld:	contains constant filed or not
	hasFlexArray:	contains flexible array(array with size 0) or not,
					the flexible array must be last field.
 *********************************************************/
typedef struct recordType
{
	TYPE_COMMON
	char *id; 
	Field flds; 
	Field *tail; 
	int hasConstFld : 16;
	int hasFlexArray : 16;
	//for test whether it is incomplete type.
	/********************************************
		struct A;	//	It is considered an incomplete type here. So sizeof(struct A) is illegal.
		void f(int a[]){
			printf("%d \n",sizeof(a));
			printf("sizeof(struct A) = %d  \n",sizeof(struct A));------- incomplete type
		}
		struct A{
			int a[10];
		};
		see IsIncompleteType(ty)
	 ********************************************/
	int complete;			//	

} *RecordType;

typedef struct enumType
{
	TYPE_COMMON
	char *id; 
	int complete;
} *EnumType;
/***************************************
	describes the parameter information in 
	function declaration or definition.
	id:	parameter name ,can be NULL.
	ty:	parameter type
	reg:	qualified by register or not
 ***************************************/
typedef struct parameter
{
	char *id; 
	Type ty; 
	int  reg; 
} *Parameter;
/**********************************************
 The meaning of hasProto: 	see K&R	 A.10.1
 (1)	
 new-style definition:
 	int max(int a,int b,int c){
 	}
 old-style definiition:
 	
 	// see K&R A.8.6.3 	
 	// In the old-style declarator, the identifier list must be absent unless
 	// the declarator is used in the head of a function defition.
 	// No information about the types of the parameters is supplied by the declaration
 	// T D1(id-list);
 	int max(a,b,c)	// This is a special old-style function declaration, because it appears in definition.
 	int a,b,c;
 	{
 		
 	}
 new-style function declaration:
 	int max(int a, int b, int c);
 old-style function declation:
 	int max();	// none of parameters type are specified.
 (2)
 	no proto:  we only know function returns int. It could be used as max(2,3), max(2,3,4)  in fact.
 	has proto:	max(2,3,4) is right, max(2,3) wrong.
 (3) hasProto:	has function prototype or not.
 	hasEllipse:	has variable argument or not
 	params:		parameter set
 **********************************************/
typedef struct signature
{
	int hasProto   : 16; 
	int hasEllipsis : 16;  
	Vector params;
} *Signature;

typedef struct functionType
{
	TYPE_COMMON
	Signature sig; 
} *FunctionType;

#define T(categ) (Types + categ)

#define IsIntegType(ty)    (ty->categ <= ENUM)
#define IsUnsigned(ty)	   (ty->categ & 0x1)
// Here, real-type means floating number.
#define IsRealType(ty)	   (ty->categ >= FLOAT && ty->categ <= LONGDOUBLE)
#define IsArithType(ty)    (ty->categ <= LONGDOUBLE)
// Scalar type is different from 'vector type' such as  struct
#define IsScalarType(ty)   (ty->categ <= POINTER)
#define IsPtrType(ty)      (ty->categ == POINTER)
#define IsRecordType(ty)   (ty->categ == STRUCT || ty->categ == UNION)
#define IsFunctionType(ty) (ty->categ == FUNCTION)

// allow pointers to object of zero size
#define IsObjectPtr(ty)     (ty->categ == POINTER &&  ty->bty->categ != FUNCTION)

#define IsIncompletePtr(ty) (ty->categ == POINTER && ty->bty->size == 0)
#define IsVoidPtr(ty)       (ty->categ == POINTER && ty->bty->categ == VOID)
#define NotFunctionPtr(ty)  (ty->categ == POINTER && ty->bty->categ != FUNCTION)

#define BothIntegType(ty1, ty2)   (IsIntegType(ty1) && IsIntegType(ty2))
#define BothArithType(ty1, ty2)   (IsArithType(ty1) && IsArithType(ty2))
#define BothScalarType(ty1, ty2)  (IsScalarType(ty1) && IsScalarType(ty2))
#define IsCompatiblePtr(ty1, ty2) (IsPtrType(ty1) && IsPtrType(ty2) &&  \
                                   IsCompatibleType(Unqual(ty1->bty), Unqual(ty2->bty)))

	/**************************************************************
		int arr[] = {
			30,40,50
		}
		arr is not incomplete type, for its size is not 0.	
	 **************************************************************/


#define	IGNORE_ZERO_SIZE_ARRAY	1
int IsZeroSizeArray(Type ty);
int IsIncompleteEnum(Type ty);	
int IsIncompleteRecord(Type ty);
int IsIncompleteType(Type ty, int ignoreZeroArray);

const char * GetCategName(int categ);


int TypeCode(Type ty);
Type Enum(char *id);
Type Qualify(int qual, Type ty);
Type Unqual(Type ty);
Type PointerTo(Type ty);
Type ArrayOf(int len, Type ty);
Type FunctionReturn(Type ty, Signature sig);
Type Promote(Type ty);

Type  StartRecord(char *id, int categ);
Field AddField(Type ty, char *id, Type fty, int bits);
Field LookupField(Type ty, char *id);


void EndRecord(Type ty, Coord coord);


int  IsCompatibleType(Type ty1, Type ty2);
Type CompositeType(Type ty1, Type ty2);
Type CommonRealType(Type ty1, Type ty2);
Type AdjustParameter(Type ty);
char* TypeToString(Type ty);

void SetupTypeSystem(void);

extern Type DefaultFunctionType;
extern Type WCharType;
extern struct type Types[VOID - CHAR + 1];

#endif
