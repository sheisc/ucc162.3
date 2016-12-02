#include <stdio.h>
#include "lex.h"
#include "expr.h"
#include "decl.h"
#include "stmt.h"

static const char * srcCode = "{int (*f(int,int,int))[4];}";
static char NextCharFromMem(void){
	int ch = *srcCode;
	srcCode++;
	if(ch == 0){
		return (char)EOF_CH;
	}else{
		return (char)ch;
	}
}

static char NextCharFromStdin(void){
	int ch = fgetc(stdin);
	if(ch == EOF){
		return (char)EOF_CH;
	}else{
		return (char)ch;
	}
}

int main(){

	AstStmtNodePtr stmt = NULL;
	

	InitLexer(NextCharFromStdin);
	NEXT_TOKEN;
	stmt = CompoundStatement();
	Expect(TK_EOF);
	VisitStatementNode(stmt);
	

	return 0;
}


