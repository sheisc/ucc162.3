#include "ucl.h"
#include "lex.h"
#include "keyword.h"

#include <time.h>
#include <stdlib.h>
#include <locale.h>
#include <wchar.h>
#include "config.h"

#define CURSOR      (Input.cursor)
#define LINE        (Input.line)
#define LINEHEAD    (Input.lineHead)

typedef int (*Scanner)(void);

static unsigned char *PeekPoint;
static union value    PeekValue;
static struct coord   PeekCoord;
static Scanner        Scanners[256];

union value  TokenValue;
struct coord TokenCoord;
struct coord PrevCoord;
/**
	TOKEN(TK_TYPEDEF,   "typedef")
	TOKEN(TK_CONST,     "const")
 */
char* TokenStrings[] = 
{
#define TOKEN(k, s) s,
#include "token.h"
#undef  TOKEN
};

/**
	Only when *CURSOR is 0xFF, 
	and 
	CURSOR points to the position behind the last byte.
 */
#define	IS_EOF(cur)	 (*(cur) == END_OF_FILE && ((cur)-Input.base) == Input.size)

/**
 * Scans preprocessing directive which specify the line number and filename such as:
 * # line 6 "C:\\Program Files\\Visual Stduio 6\\VC6\\Include\\stdio.h" or
 * # 6 "/usr/include/stdio.h"
 * Ignores other preprocessing directive.
 */
 /**
	hello.c	--->  hello.i		(On Linux)

	hello.c
		Line1:	#include <stdio.h>
		Line2:	int f(int n){
	hello.i
		Line655:	# 2 "hello.c" 2
  */
static void ScanPPLine(void)
{
	int line = 0;

	CURSOR++;
	while (*CURSOR == ' ' || *CURSOR == '\t')
	{
		CURSOR++;
	}
	/* # 2 "hello.c" 2		on Linux */
	if (IsDigit(*CURSOR))
	{
		goto read_line;
	}
	/* # line 6 "C:\\Program Files\\Visual Stduio 6\\VC6\\Include\\stdio.h" 	on Windows */
	else if (strncmp(CURSOR, "line", 4) == 0)
	{
		CURSOR += 4;
		while (*CURSOR == ' ' || *CURSOR == '\t')
		{
			CURSOR++;
		}
read_line:
		/* line number */
		while (IsDigit(*CURSOR))
		{
			line = 10 * line + *CURSOR - '0';
			CURSOR++;
		}
		TokenCoord.ppline = line - 1;
		/* skip white space */
		while (*CURSOR == ' ' || *CURSOR == '\t')
		{
			CURSOR++;
		}
		/* get the filename: "hello.c " --->  hello.c */
		TokenCoord.filename = ++CURSOR;
		while (*CURSOR != '"' && !IS_EOF(CURSOR)&& *CURSOR != '\n')
		{
			CURSOR++;
		}		
		TokenCoord.filename = InternName(TokenCoord.filename, (char *)CURSOR - TokenCoord.filename);
	}
	while (*CURSOR != '\n' && !IS_EOF(CURSOR))	
	{
		CURSOR++;
	}
}


#ifdef _WIN32
#define	DECLSPEC_PREFIX			"__declspec("
#define	WIN32_ASM_PREFIX			"__asm"
int SkipWin32Declares(void){
	/**
		to skip the "__declspec(...)"  generated by cl.exe on Windows
	*/
	if (strncmp(CURSOR, DECLSPEC_PREFIX, strlen(DECLSPEC_PREFIX)) == 0){
			int lpCount = 1;
			CURSOR += strlen(DECLSPEC_PREFIX);
			/* We assume that all parenthesis are in pairs.  */
			while(lpCount != 0){
				if(*CURSOR == '('){
					lpCount++;
				}else if(*CURSOR == ')'){
					lpCount--;
				}
				CURSOR++;
			}
			return 1;
	}
	/**
		we need inline asm mechanism for UCC in the future.
		Right now, just skip all these inline assebly codes.
		see Line 878 in "C:\\Program Files (x86)\\Windows Kits\\8.1\\Include\\um\\winnt.h"
	*/
	if (strncmp(CURSOR, WIN32_ASM_PREFIX, strlen(WIN32_ASM_PREFIX)) == 0){
			int rbraceCount = 0;
			unsigned char * asmStart = CURSOR;
			CURSOR += strlen(WIN32_ASM_PREFIX);
			/* skip until we get the end of inline function definition, '}' */
			while(1){
				if(*CURSOR == '{'){
					rbraceCount--;
				}else if(*CURSOR == '}'){
					rbraceCount++;
					if(rbraceCount == 1){
						break;
					}
				}
				CURSOR++;
			}
			return 1;
	}	
	return 0;
}
#endif



static void SkipWhiteSpace(void)
{
	int ch;

again:
	ch = *CURSOR;
	while (ch == '\t' || ch == '\v' || ch == '\f' || ch == ' ' ||
	       ch == '\r' || ch == '\n' || ch == '/'  || ch == '#')
	{
		switch (ch)
		{
		case '\n':
			TokenCoord.ppline++;
			LINE++;
			LINEHEAD = ++CURSOR;
			break;

		case '#':	/*  # 2 "hello.c"		*.i files		generated by preprocesser */
			ScanPPLine();
			break;

		case '/':	/* comments */
			/**
				C Style comment or C++ style comment
				On Linux, 
				In fact , all the comments have been eaten by preprocesser.
				The UCL compiler won't encounter any comments.
			 */
			if (CURSOR[1] != '/' && CURSOR[1] != '*')
				return;
			CURSOR++;
			if (*CURSOR == '/')
			{
				CURSOR++;
				while (*CURSOR != '\n' && !IS_EOF(CURSOR))
				{
					CURSOR++;
				}
			}
			else
			{
				CURSOR += 1;
				while (CURSOR[0] != '*' || CURSOR[1] != '/')
				{
					if (*CURSOR == '\n')
					{
						TokenCoord.ppline++;
						LINE++;
					}
					else if (IS_EOF(CURSOR)|| IS_EOF(&CURSOR[1]))
					{
						Error(&TokenCoord, "Comment is not closed");
						return;
					}
					CURSOR++;
				}
				CURSOR += 2;
			}
			break;

		default:
			CURSOR++;
			break;
		}
		ch = *CURSOR;
	}
#ifdef	_WIN32
	if(SkipWin32Declares()){
		goto again;
	}	
#endif
	if (ExtraWhiteSpace != NULL)
	{
		char *p;
		/* ignore the unknown strings, that is , ExtraWhiteSpace. */
		FOR_EACH_ITEM(char*, p, ExtraWhiteSpace)
			if (strncmp(CURSOR, p, strlen(p)) == 0)
			{	
				CURSOR += strlen(p);
				goto again;
			}
		ENDFOR
	}
}

static int ScanEscapeChar(int wide)
{

	int v = 0, overflow = 0;

	CURSOR++;
	switch (*CURSOR++)
	{
	case 'a':
		return '\a';

	case 'b':
		return '\b';

	case 'f':
		return '\f';

	case 'n':
		return '\n';

	case 'r':
		return '\r';

	case 't':
		return '\t';

	case 'v':
		return '\v';

	case '\'':
	case '"':
	case '\\':
	case '\?':
		return *(CURSOR - 1);

	case 'x':		/*		\xhh	hexical */
		if (! IsHexDigit(*CURSOR))
		{
			Error(&TokenCoord, "Expect hex digit");
			return 'x';
		}
		v = 0;
		while (IsHexDigit(*CURSOR))
		{
			/**
				Bug? 	
				if(v >> (WCharType->size * 8-4 )) 
				(1) WCharType->size == 2
					0xABCD * 16 + value --> overflow
					0x0ABC is OK.
				(2) WCharType->size == 4
					0x12345678  * 16 + value --> overflow
					0x01234567 is OK.
			*/
			if (v >> (WCharType->size*8 - 4)){
				overflow = 1;
			}			
			/*  v= v * 16 + value,  value : 0-9  A-F */
			if (IsDigit(*CURSOR))
			{
				v = (v << 4) + *CURSOR - '0';
			}
			else
			{
				v = (v << 4) + ToUpper(*CURSOR) - 'A' + 10;
			}
			CURSOR++;
		}
		if (overflow || (! wide && v > 255))
		{
			Warning(&TokenCoord, "Hexademical espace sequence overflow");
		}
		return v;

	case '0': case '1': case '2': case '3':
	case '4': case '5': case '6': case '7':	/* \ddd	octal */
		v = *(CURSOR - 1) - '0';
		if (IsOctDigit(*CURSOR))
		{
			v = (v << 3) + *CURSOR++ - '0';
			if (IsOctDigit(*CURSOR))
				v = (v << 3) + *CURSOR++ - '0';
		}
		return v;

	default:
		Warning(&TokenCoord, "Unrecognized escape sequence:\\%c", *CURSOR);
		return *CURSOR;
	}
}
/* return keyword or TK_ID */
static int FindKeyword(char *str, int len)
{
	struct keyword *p = NULL;
	/* index is 0 when "__int64", see keyword.h	static struct keyword keywords_[] */
	int index = 0;

	if (*str != '_')
		index = ToUpper(*str) - 'A' + 1;

	p = keywords[index];
	while (p->name)
	{
		if (p->len == len && strncmp(str, p->name, len) == 0)
			break;
		p++;
	}	
	return p->tok;
}

static int ScanIntLiteral(unsigned char *start, int len, int base)
{
	unsigned char *p =  start;
	unsigned char *end = start + len;
	unsigned int i[2] = {0, 0};
	int tok = TK_INTCONST;
	int d = 0;
	int carry0 = 0, carry1 = 0;
	int overflow = 0;

	while (p != end)
	{
		if (base == 16)
		{
			if ((*p >= 'A' && *p <= 'F') ||
				(*p >= 'a' && *p <= 'f'))
			{
				d = ToUpper(*p) - 'A' + 10;
			}
			else
			{
				d = *p - '0';
			}
		}
		else
		{
			d = *p - '0';
		}
		/**
			treat i[1],i[0] as 64 bit integer.			
		*/
		switch (base)
		{
		case 16:
			carry0 = HIGH_4BIT(i[0]);
			carry1 = HIGH_4BIT(i[1]);
			i[0] = i[0] << 4;
			i[1] = i[1] << 4;
			break;

		case 8:
			carry0 = HIGH_3BIT(i[0]);
			carry1 = HIGH_3BIT(i[1]);
			i[0] = i[0] << 3;
			i[1] = i[1] << 3;
			break;

		case 10:
			{
				unsigned int t1, t2;
				/* number * 10 = number * 8 + number * 2 = (number << 3) + (number << 1) */
				carry0 = HIGH_3BIT(i[0]) + HIGH_1BIT(i[0]);
				carry1 = HIGH_3BIT(i[1]) + HIGH_1BIT(i[1]);
				t1 = i[0] << 3;
				t2 = i[0] << 1;
				if (t1 > UINT_MAX - t2)	/* In maths:  t1 + t2 > UINT_MAX */
				{
					carry0++;
				}
				i[0] = t1 + t2;
				t1 = i[1] << 3;
				t2 = i[1] << 1;
				if (t1 > UINT_MAX - t2)
				{
					carry1++;
				}
				i[1] = t1 + t2;
			}
			break;
		}
		if (i[0] > UINT_MAX - d)	/* for decimal, i[0] + d maybe greater than UINT_MAX */
		{
			carry0 += i[0] - (UINT_MAX - d);
		}
		if (carry1 || (i[1] > UINT_MAX - carry0))
		{
			overflow = 1;
		}
		i[0] += d;
		i[1] += carry0;
		p++;
	}
	/**
		overflow != 0:
			out of 64 bit bound
		i[1] != 0
			out of 32 bit bound
	 */
	if (overflow || i[1] != 0)		
	{
		Warning(&TokenCoord, "Integer literal is too big");
	}

	TokenValue.i[1] = 0;
	TokenValue.i[0] = i[0];
	tok = TK_INTCONST;
	/**
		12345678U
		12345678u
	 */
	if (*CURSOR == 'U' || *CURSOR == 'u')
	{
		CURSOR++;
		if (tok == TK_INTCONST)
		{
			tok = TK_UINTCONST;
		}
		else if (tok == TK_LLONGCONST)
		{
			tok = TK_ULLONGCONST;
		}
	}
	/**
		12345678UL
		12345678L
	 */
	if (*CURSOR == 'L' || *CURSOR == 'l')
	{
		CURSOR++;
		if (tok == TK_INTCONST)
		{
			tok = TK_LONGCONST;
		}
		else if (tok == TK_UINTCONST)
		{
			tok = TK_ULONGCONST;
		}
		if (*CURSOR == 'L' || *CURSOR == 'l')	/* LL  long long int */
		{
			CURSOR++;
			if (tok < TK_LLONGCONST)
			{
				tok = TK_LLONGCONST;
			}
		}
	}

	return tok;
}
/**
	@start  points to the beginngin of the float
	@CURSOR points to the possible  \. or E or e
	.123		, 123.456
 */
static int ScanFloatLiteral(unsigned char *start)
{
	double d;
	/**
		Just check the optional fragment part and the 
		exponent part.
		The value of float number is determined by 
		strtod().
	 */
	if (*CURSOR == '.')
	{
		CURSOR++;
		while (IsDigit(*CURSOR))
		{
			CURSOR++;
		}
	}
	if (*CURSOR == 'e' || *CURSOR == 'E')
	{
		CURSOR++;
		if (*CURSOR == '+' || *CURSOR == '-')
		{
			CURSOR++;
		}
		if (! IsDigit(*CURSOR))
		{
			Error(&TokenCoord, "Expect exponent value");
		}
		else
		{
			while (IsDigit(*CURSOR))
			{
				CURSOR++;
			}
		}
	}

	errno = 0;
	d = strtod((char *)start, NULL);
	if (errno == ERANGE)
	{
		Warning(&TokenCoord, "Float literal overflow");
	}
	TokenValue.d = d;
	/* single precision float:  123.456f */
	if (*CURSOR == 'f' || *CURSOR == 'F')
	{
		CURSOR++;
		TokenValue.f = (float)d;
		return TK_FLOATCONST;
	}
	else if (*CURSOR == 'L' || *CURSOR == 'l')	/* long double */
	{
		CURSOR++;
		return TK_LDOUBLECONST;
	}
	else
	{
		return TK_DOUBLECONST;
	}
}
/*  float / int */
static int ScanNumericLiteral(void)
{
	unsigned char *start = CURSOR;
	int base = 10;
	
	if (*CURSOR == '.')	/* float  .123 */
	{
		return ScanFloatLiteral(start);
	}

	if (*CURSOR == '0' && (CURSOR[1] == 'x' || CURSOR[1] == 'X'))	/* hexical  0x123ABC / 0X123ABC */
	{
		CURSOR += 2;
		start = CURSOR;
		base = 16;
		if (! IsHexDigit(*CURSOR))
		{
			Error(&TokenCoord, "Expect hex digit");
			TokenValue.i[0] = 0;
			return TK_INTCONST;
		}
		while (IsHexDigit(*CURSOR))	/* 123ABC of  0x123ABC */
		{
			CURSOR++;
		}
	}
	else if (*CURSOR == '0')	/* octal		01234567 */
	{
		CURSOR++;
		base = 8;
		while (IsOctDigit(*CURSOR))	/* 1234567 of 0123567 */
		{
			CURSOR++;
		}
	}
	else	/* decimal	123456789 */
	{
		CURSOR++;
		while (IsDigit(*CURSOR))
		{
			CURSOR++;
		}
	}
	/**
		(1)  if number starts with 0x
		or
		(2) if  *CURSOR are not part of a float number
		we are sure that we encounter a int literal.  base = 8/10/16
	 */
	if (base == 16 || (*CURSOR != '.' && *CURSOR != 'e' && *CURSOR != 'E'))
	{
		return ScanIntLiteral(start, (int)(CURSOR - start), base);
	}
	else
	{
		/* 123.456 */
		return ScanFloatLiteral(start);
	}
}

static int ScanCharLiteral(void)
{

	UCC_WC_T ch = 0;
	size_t n = 0;
	int count = 0;
	int wide = 0;

	if (*CURSOR == 'L')	/* wide char			L'a'		L'\t'	*/
	{
		CURSOR++;
		wide = 1;
	}
	CURSOR++;			/* skip \' */
	if(*CURSOR == '\''){
		Error(&TokenCoord, "empty character constant");
	}else if(*CURSOR == '\n' || IS_EOF(CURSOR)){
		Error(&TokenCoord,"missing terminating ' character");
	}else{
		if(*CURSOR == '\\'){
			ch = (UCC_WC_T) ScanEscapeChar(wide);
		}else{
			if(wide){
				n = mbrtowc(&ch, CURSOR, MB_CUR_MAX, 0);
				if(n > 0){
					CURSOR += n;
				}
				/* PRINT_DEBUG_INFO(("%x %x",n,ch)); */
			}else{
				ch = *CURSOR;
				CURSOR++;
			}
		}
		while (*CURSOR != '\'')		/* L'abc',  skip the redundant characters */
		{
			if (*CURSOR == '\n' || IS_EOF(CURSOR))
				break;
			CURSOR++;
			count++;
		}
	}


	if (*CURSOR != '\'')
	{
		Error(&TokenCoord, "missing terminating ' character");
		goto end_char;
	}

	CURSOR++;
	if (count > 0)
	{
		Warning(&TokenCoord, "Too many characters");
	}

end_char:
	TokenValue.i[0] = ch;
	TokenValue.i[1] = 0;

	return TK_INTCONST;

}

static int ScanStringLiteral(void)	/* "abc"  or L"abc" */
{

	char tmp[512];
	char *cp = tmp;
	UCC_WC_T *wcp = (UCC_WC_T *)tmp;
	int wide = 0;
	int len = 0;
	int maxlen = 512;
	UCC_WC_T ch = 0;
	String str;
	size_t n = 0;
	
	CALLOC(str);
	
	if (*CURSOR == 'L')
	{
		CURSOR++;
		wide = 1;
		/* char tmp[512] --> int tmp[512/sizeof(int)] */
		maxlen /= sizeof(UCC_WC_T);
	}
	CURSOR++;			/* skip " */

next_string:
	while (*CURSOR != '"')
	{
		if (*CURSOR == '\n' || IS_EOF(CURSOR))
			break;
		if(*CURSOR == '\\'){
			ch =  (UCC_WC_T)ScanEscapeChar(wide);
		}else{
			if(wide){
				n = mbrtowc(&ch, CURSOR, MB_CUR_MAX, 0);
				if(n > 0){
					CURSOR += n;
				}else{
					ch = *CURSOR;
					CURSOR++;
				}
				/* PRINT_DEBUG_INFO(("%x %x",n,ch)); */
			}else{
				ch = *CURSOR;
				CURSOR++;
			}
		}
		if (wide){
			wcp[len] = ch;
		}else{
			cp[len] = (char)ch;
		}
		len++;
		if (len >= maxlen){
			AppendSTR(str, tmp, len, wide);
			len = 0;
		}
	}

	if (*CURSOR != '"')	{
		Error(&TokenCoord, "Expect \"");
		goto end_string;
	}

	CURSOR++;		/* skip " */
	SkipWhiteSpace();
	if (CURSOR[0] == '"')		/* "abc"		"123"	---> "abc123" */
	{
		if (wide == 1)
		{
			Error(&TokenCoord, "String wideness mismatch");
		}
		CURSOR++;
		goto next_string;
	}
	else if (CURSOR[0] == 'L' && CURSOR[1] == '"')	/* L"abc"	L"123"	--> L"abc123" */
	{
		if (wide == 0)
		{
			Error(&TokenCoord, "String wideness mismatch");
		}
		CURSOR += 2;
		goto next_string;
	}

end_string:
	AppendSTR(str, tmp, len, wide);
	TokenValue.p = str;

	return wide ? TK_WIDESTRING : TK_STRING;


}
/* parse string starting with char */
static int ScanIdentifier(void)
{
	unsigned char *start = CURSOR;
	int tok;

	if (*CURSOR == 'L')	/* special case :  wide char/string */
	{
		if (CURSOR[1] == '\'')
		{
			return ScanCharLiteral();		/* L'a'	wide char */
		}
		if (CURSOR[1] == '"')
		{
			return ScanStringLiteral();	/* 	L"wide string"  */
		}
	}
	/* letter(letter|digit)* */
	CURSOR++;
	while (IsLetterOrDigit(*CURSOR))
	{
		CURSOR++;
	}

	tok = FindKeyword((char *)start, (int)(CURSOR - start));
	if (tok == TK_ID)
	{
		TokenValue.p = InternName((char *)start, (int)(CURSOR - start));
	}

	return tok;
}

static int ScanPlus(void)
{
	CURSOR++;
	if (*CURSOR == '+')
	{
		CURSOR++;
		return TK_INC;			/* ++ */
	}
	else if (*CURSOR == '=')
	{
		CURSOR++;
		return TK_ADD_ASSIGN;	/* += */
	}
	else
	{
		return TK_ADD;			/* + */
	}
}

static int ScanMinus(void)
{
	CURSOR++;
	if (*CURSOR == '-')
	{
		CURSOR++;
		return TK_DEC;			/* --  */
	}
	else if (*CURSOR == '=')
	{
		CURSOR++;
		return TK_SUB_ASSIGN;	/* -=  */
	}
	else if (*CURSOR == '>')
	{
		CURSOR++;
		return TK_POINTER;		/* ->  */
	}
	else
	{
		return TK_SUB;			/* -  */
	}
}

static int ScanStar(void)
{
	CURSOR++;
	if (*CURSOR == '=')
	{
		CURSOR++;
		return TK_MUL_ASSIGN;		/* *=  */
	}
	else
	{
		return TK_MUL;				/* *  */
	}
}

static int ScanSlash(void)
{
	CURSOR++;
	if (*CURSOR == '=')
	{
		CURSOR++;
		return TK_DIV_ASSIGN;		/*	 /=  */
	}
	else
	{
		return TK_DIV;				/* /  */
	}
}

static int ScanPercent(void)
{
	CURSOR++;
	if (*CURSOR == '=')
	{
		CURSOR++;
		return TK_MOD_ASSIGN;		/* %= */
	}
	else
	{
		return TK_MOD;				/* %  */
	}
}

static int ScanLess(void)
{
	CURSOR++;
	if (*CURSOR == '<')
	{
		CURSOR++;
		if (*CURSOR == '=')
		{
			CURSOR++;
			return TK_LSHIFT_ASSIGN;		/* <<= */
		}
		return TK_LSHIFT;		/* <<  */
	}
	else if (*CURSOR == '=')
	{
		CURSOR++;
		return TK_LESS_EQ;		/* <=  */
	}
	else
	{
		return TK_LESS;			/* <  */
	}
}

static int ScanGreat(void)
{
	CURSOR++;
	if (*CURSOR == '>')
	{
		CURSOR++;
		if (*CURSOR == '=')		/* >>=  */
		{
			CURSOR++;
			return TK_RSHIFT_ASSIGN;
		}
		return TK_RSHIFT;		/* >>  */
	}
	else if (*CURSOR == '=')
	{
		CURSOR++;
		return TK_GREAT_EQ;			/* >=  */
	}
	else
	{
		return TK_GREAT;		/* >  */
	}
}

static int ScanExclamation(void)
{
	CURSOR++;
	if (*CURSOR == '=')		/* != */
	{
		CURSOR++;
		return TK_UNEQUAL;
	}
	else		/* ! */
	{
		return TK_NOT;
	}
}

static int ScanEqual(void)
{
	CURSOR++;
	if (*CURSOR == '=')		/* ==  */
	{
		CURSOR++;
		return TK_EQUAL;
	}
	else	/* =  */
	{
		return TK_ASSIGN;
	}
}

static int ScanBar(void)
{
	CURSOR++;
	if (*CURSOR == '|')		/* || */
	{
		CURSOR++;
		return TK_OR;
	}
	else if (*CURSOR == '=')	/* |=  */
	{
		CURSOR++;
		return TK_BITOR_ASSIGN;
	}
	else	/* | */
	{
		return TK_BITOR;
	}
}

static int ScanAmpersand(void)
{
	CURSOR++;
	if (*CURSOR == '&')		/* && */
	{
		CURSOR++;
		return TK_AND;
	}
	else if (*CURSOR == '=')	/* &= */
	{
		CURSOR++;
		return TK_BITAND_ASSIGN;
	}
	else		/* & */
	{
		return TK_BITAND;
	}
}

static int ScanCaret(void)	/* ^	exclusive or */
{
	CURSOR++;
	if (*CURSOR == '=')
	{
		CURSOR++;
		return TK_BITXOR_ASSIGN;
	}
	else
	{
		return TK_BITXOR;
	}
}

static int ScanDot(void)
{
	if (IsDigit(CURSOR[1]))	/* .123		float number */
	{
		return ScanFloatLiteral(CURSOR);
	}
	else if (CURSOR[1] == '.' && CURSOR[2] == '.')	/*  ... 	variable parameters. */
	{
		CURSOR += 3;
		return TK_ELLIPSIS;
	}
	else	/* just a simple dot */
	{
		CURSOR++;
		return TK_DOT;
	}
}

#define SINGLE_CHAR_SCANNER(t) \
static int Scan##t(void)       \
{                              \
    CURSOR++;                  \
    return TK_##t;             \
}

SINGLE_CHAR_SCANNER(LBRACE)
SINGLE_CHAR_SCANNER(RBRACE)
SINGLE_CHAR_SCANNER(LBRACKET)
SINGLE_CHAR_SCANNER(RBRACKET)
SINGLE_CHAR_SCANNER(LPAREN)
SINGLE_CHAR_SCANNER(RPAREN)
SINGLE_CHAR_SCANNER(COMMA)
SINGLE_CHAR_SCANNER(SEMICOLON)
SINGLE_CHAR_SCANNER(COMP)
SINGLE_CHAR_SCANNER(QUESTION)
SINGLE_CHAR_SCANNER(COLON)
/* 	illegal character */
static int ScanBadChar(void)
{
	Error(&TokenCoord, "illegal character:\\x%x", *CURSOR);
	/**
		move CURSOR a step forward, 
		because GetNextToken() doesn't move CURSOR in this case.
	 */
	CURSOR++;
	return GetNextToken();
}

static int ScanEOF(void)
{
	if(!IS_EOF(CURSOR)){
		return ScanBadChar();
	}
	return TK_END;
}

void SetupLexer(void)
{
	int i;
	/* for wchar_t */
	setlocale(LC_CTYPE, "");

	for (i = 0; i < END_OF_FILE + 1; i++)
	{
		if (IsLetter(i))	/* [a-z A-Z _ ] */
		{
			Scanners[i] = ScanIdentifier;
		}
		else if (IsDigit(i))
		{
			Scanners[i] = ScanNumericLiteral;
		}
		else
		{
			Scanners[i] = ScanBadChar;
		}
	}

	Scanners[END_OF_FILE] = ScanEOF;
	Scanners['\''] = ScanCharLiteral;	/* wide chars/strings are parsed in ScanIdentifier(void) */
	Scanners['"']  = ScanStringLiteral;
	Scanners['+']  = ScanPlus;
	Scanners['-']  = ScanMinus;
	Scanners['*']  = ScanStar;
	Scanners['/']  = ScanSlash;
	Scanners['%']  = ScanPercent;
	Scanners['<']  = ScanLess;
	Scanners['>']  = ScanGreat;
	Scanners['!']  = ScanExclamation;
	Scanners['=']  = ScanEqual;
	Scanners['|']  = ScanBar;
	Scanners['&']  = ScanAmpersand;
	Scanners['^']  = ScanCaret;
	Scanners['.']  = ScanDot;
	/* see Macro SINGLE_CHAR_SCANNER(t) */
	Scanners['{']  = ScanLBRACE;
	Scanners['}']  = ScanRBRACE;
	Scanners['[']  = ScanLBRACKET;
	Scanners[']']  = ScanRBRACKET;
	Scanners['(']  = ScanLPAREN;
	Scanners[')']  = ScanRPAREN;
	Scanners[',']  = ScanCOMMA;
	Scanners[';']  = ScanSEMICOLON;
	Scanners['~']  = ScanCOMP;
	Scanners['?']  = ScanQUESTION;
	Scanners[':']  = ScanCOLON;
	/**
		If ExtraKeywords is NULL, all the special keywords in 'keywords_' are 
		inactivated.
	 */
	if (ExtraKeywords != NULL)
	{
		char *str;
		struct keyword *p;
		/**
			(1)
			see keyword.h:
			static struct keyword keywords_[] = 
			{
				{"__int64", 0, TK_INT64},-----------> It is inactivated when the length is 0.
				{NULL,      0, TK_ID}
			};
			p->len is 0 after initializing.
			(2)
			ExtraKeywords is a Vector
			(3)
			The following code set the p->len.
		*/
		FOR_EACH_ITEM(char*, str, ExtraKeywords)
			p = keywords_;
			while (p->name)
			{
				if (strcmp(str, p->name) == 0)
				{
					p->len = strlen(str);
					break;
				}
				p++;
			}
		ENDFOR
	}
}

int GetNextToken(void)
{
	int tok;

	PrevCoord = TokenCoord;
	SkipWhiteSpace();
	TokenCoord.line = LINE;	/* line number in the *.i for C compiler */
	TokenCoord.col  = (int)(CURSOR - LINEHEAD + 1);
	/* use function pointer table to avoid a large switch statement. */
	tok = (*Scanners[*CURSOR])();
	return tok;
}
/* mark() */
void BeginPeekToken(void)
{
	PeekPoint = CURSOR;
	PeekValue = TokenValue;
	PeekCoord = TokenCoord;
}
/* reset() */
void EndPeekToken(void)
{
	CURSOR = PeekPoint;
	TokenValue = PeekValue;
	TokenCoord = PeekCoord;
}


