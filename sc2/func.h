#ifndef	FUNC_H
#define	FUNC_H
#include "stmt.h"

#define MAX_PARAMETERS_CNT   10
#define MAX_LOCAL_VARS_CNT   30

typedef struct astFuncDefNode{
	TokenKind op;
	Value value;
	/////////////////////////////
	// for parameters
	struct astNode paras[MAX_PARAMETERS_CNT];
	int para_cnt;
	// for local variables
		// for parameters
	struct astNode local_vars[MAX_LOCAL_VARS_CNT];
	int local_vars_cnt;

    // funcBody
    AstStmtNodePtr funcBody;
    // next function
	struct astFuncDefNode * next;

} * AstFuncDefNodePtr;

AstFuncDefNodePtr FunctionDefinitions(void);
void EmitFuncDefNode(AstFuncDefNodePtr func);

AstFuncDefNodePtr GetCurFuncDef(void);
void SetCurFuncDef(AstFuncDefNodePtr func);

#endif
