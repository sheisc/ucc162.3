struct keyword
{
	char *name;
	int len;
	int tok;
};

static struct keyword keywords_[] = 
{
	{"__int64", 0, TK_INT64},
	{NULL,      0, TK_ID}
};

static struct keyword keywordsA[] =
{
	{"auto", 4, TK_AUTO},
	{NULL,   0, TK_ID}
};

static struct keyword keywordsB[] = 
{
	{"break", 5, TK_BREAK},
	{NULL,    0, TK_ID}
};

static struct keyword keywordsC[] = 
{
	{"case",     4, TK_CASE},
	{"char",     4, TK_CHAR},
	{"const",    5, TK_CONST},
	{"continue", 8, TK_CONTINUE},
	{NULL,       0, TK_ID}
};

static struct keyword keywordsD[] =
{
	{"default", 7, TK_DEFAULT},
	{"do",      2, TK_DO},
	{"double",  6, TK_DOUBLE},
	{NULL,      0, TK_ID}
};

static struct keyword keywordsE[] =
{
	{"else",   4, TK_ELSE},
	{"enum",   4, TK_ENUM},
	{"extern", 6, TK_EXTERN},
	{NULL,     0, TK_ID}
};

static struct keyword keywordsF[] =
{
	{"float", 5, TK_FLOAT},
	{"for",   3, TK_FOR},
	{NULL,    0, TK_ID}
};

static struct keyword keywordsG[] = 
{
	{"goto", 4, TK_GOTO},
	{NULL,   0, TK_ID}
};

static struct keyword keywordsH[] =
{
	{NULL, 0, TK_ID}
};

static struct keyword keywordsI[] = 
{
	{"if",  2, TK_IF},
	{"int", 3, TK_INT},
	{NULL,  0, TK_ID}
};

static struct keyword keywordsJ[] = 
{
	{NULL, 0, TK_ID}
};

static struct keyword keywordsK[] = 
{
	{NULL, 0, TK_ID}
};

static struct keyword keywordsL[] = 
{
	{"long", 4,	TK_LONG},
	{NULL,   0, TK_ID}
};

static struct keyword keywordsM[] = 
{
	{NULL, 0, TK_ID}
};

static struct keyword keywordsN[] = 
{
	{NULL, 0, TK_ID}
};

static struct keyword keywordsO[] = 
{
	{NULL, 0, TK_ID}
};

static struct keyword keywordsP[] = 
{
	{NULL, 0, TK_ID}
};

static struct keyword keywordsQ[] = 
{
	{NULL, 0, TK_ID}
};

static struct keyword keywordsR[] = 
{
	{"register", 8, TK_REGISTER},
	{"return",   6, TK_RETURN},
	{NULL,       0, TK_ID}
};

static struct keyword keywordsS[] = 
{
	{"short",  5, TK_SHORT},
	{"signed", 6, TK_SIGNED},
	{"sizeof", 6, TK_SIZEOF},
	{"static", 6, TK_STATIC},
	{"struct", 6, TK_STRUCT},
	{"switch", 6, TK_SWITCH},
	{NULL,     0, TK_ID}
};

static struct keyword keywordsT[] = 
{
	{"typedef", 7, TK_TYPEDEF},
	{NULL,      0, TK_ID}
};

static struct keyword keywordsU[] = 
{
	{"union",    5, TK_UNION},
	{"unsigned", 8, TK_UNSIGNED},
	{NULL,       0, TK_ID}
};

static struct keyword keywordsV[] = 
{
	{"void",     4, TK_VOID},
	{"volatile", 8, TK_VOLATILE},
	{NULL,       0, TK_ID}
};

static struct keyword keywordsW[] = 
{
	{"while", 5, TK_WHILE },
	{NULL, 0, TK_ID}
};

static struct keyword keywordsX[] = 
{
	{NULL, 0, TK_ID}
};

static struct keyword keywordsY[] = 
{
	{NULL, 0, TK_ID}
};

static struct keyword keywordsZ[] = 
{
	{NULL, 0, TK_ID}
};
/*********************************************
	classify keywords by their first letter,
	to speed up comparing.
	see function FindKeyword()
 *********************************************/
static struct keyword *keywords[] =
{
	keywords_, keywordsA, keywordsB, keywordsC, 
	keywordsD, keywordsE, keywordsF, keywordsG, 
	keywordsH, keywordsI, keywordsJ, keywordsK, 
	keywordsL, keywordsM, keywordsN, keywordsO, 
	keywordsP, keywordsQ, keywordsR, keywordsS, 
	keywordsT, keywordsU, keywordsV, keywordsW, 
	keywordsX, keywordsY, keywordsZ
};

