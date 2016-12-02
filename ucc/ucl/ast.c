#include "ucl.h"
#include "ast.h"

int CurrentToken;

int CurFileLineNo;
const char * CurFileName;


/**
 * Expect current token to be tok. If so, get next token; otherwise,
 * report error
 */
 // 	similar function as match() in dragon book
void Do_Expect(int tok)
{
	if (CurrentToken == tok)
	{
		NEXT_TOKEN;
		return;
	}
	fprintf(stderr,"(%s %d):",CurFileName,CurFileLineNo);
	// a common case, missing ';' on last line
	if (tok == TK_SEMICOLON && TokenCoord.line - PrevCoord.line == 1)
	{
		Do_Error(&PrevCoord, "Expect ;");
	}
	else
	{
		Do_Error(&TokenCoord, "Expect %s", TokenStrings[tok - 1]);
	}
}

/**
 * Check if current token is in a token set
 */
int CurrentTokenIn(int toks[])
{
	int *p = toks;

	while (*p)
	{
		if (CurrentToken == *p)
			return 1;
		p++;
	}

	return 0;
}

/**
 * Starting from current token, skip following tokens until
 * encountering any token in the token set toks. This function is used by
 * parser to recover from error.
 */ 
void Do_SkipTo(int toks [ ],char * einfo)
{
	int *p = toks;
	struct coord cord;

	if (CurrentTokenIn(toks) || CurrentToken == TK_END)
		return;

	cord = TokenCoord;
	while (CurrentToken != TK_END)
	{
		p = toks;
		while (*p)
		{
			if (CurrentToken == *p)
				goto sync;
			p++;
		}
		NEXT_TOKEN;
	}

sync:
	fprintf(stderr,"(%s %d):",CurFileName,CurFileLineNo);
	Do_Error(&cord, "skip to %s\n", einfo);
}
