#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include "stmt.h"
#include "decl.h"
#include "error.h"
#include "emit.h"
#include "func.h"
#include "expr.h"

#define START_OFFSET_OF_PARAS   20

static AstFuncDefNodePtr curFuncDef;

static AstFuncDefNodePtr CreateFuncDefNode(Token funcId){
	AstFuncDefNodePtr pNode = (AstFuncDefNodePtr) malloc(sizeof(struct astFuncDefNode));
	memset(pNode,0,sizeof(*pNode));
	pNode->op = TK_FUNC;
    pNode->value = funcId.value;    
	return pNode;
}

static void sanity_check(AstFuncDefNodePtr func){
    (void) func;
    // FIXME
    // check whether locals variables and parameters have the same name
}

// ID(id, id, id, ..., id) {  }
static AstFuncDefNodePtr OneFunctionDef(void){
    assert(curToken.kind == TK_ID);
    AstFuncDefNodePtr func = CreateFuncDefNode(curToken);
    NEXT_TOKEN;
    Expect(TK_LPAREN);
    int para_cnt = 0;
    ResetFrameSize();
    while(curToken.kind == TK_ID){
        memset(&func->paras[para_cnt], 0, sizeof(func->paras[para_cnt]));
        func->paras[para_cnt].op = TK_ID;
        func->paras[para_cnt].value = curToken.value;
        func->paras[para_cnt].offset = START_OFFSET_OF_PARAS + para_cnt * CPU_WORD_SIZE_IN_BYTES;
        para_cnt++;
        NEXT_TOKEN;
        if(curToken.kind == TK_COMMA){
            NEXT_TOKEN;
        }
        else if (curToken.kind == TK_RPAREN) {
            break;
        }
        else{
            Error("Illegal tokens in function definition\n");
        }
       
    }    
    Expect(TK_RPAREN);
    func->para_cnt = para_cnt;
    // To be indirectly used by EmitLocalDeclarationNode() in stmt.c
    SetCurFuncDef(func);
    func->funcBody = CompoundStatement();
    sanity_check(func);
    return func;
}

// ID(id, id, id, ..., id) {  }
AstFuncDefNodePtr FunctionDefinitions(void){
	AstFuncDefNodePtr first = NULL, cur = NULL, pre = NULL;
	AstStmtNodePtr * funcBody;
	Value value;

	while(curToken.kind == TK_ID){  
        cur = OneFunctionDef();
        if(pre == NULL){
            first = cur;
        }
        else{
            pre->next = cur;
        }
        pre = cur;


	}
	return first;
}

AstFuncDefNodePtr GetCurFuncDef(void){
    return curFuncDef;
}

void SetCurFuncDef(AstFuncDefNodePtr func){
    curFuncDef = func;
}

void EmitFuncDefNode(AstFuncDefNodePtr func){
    while(func){
        SetCurFuncDef(func);
        /*********************************	
        .text
        .globl	main
        main:
        **********************************/
        EmitLabel("\n.text");
        EmitLabel(".globl	%s", func->value.name);
        EmitLabel("%s:", func->value.name);
        EmitPrologue(GetFrameSize());
        EmitStatementNode(func->funcBody);
        EmitEpilogue();
        func = func->next;
    }
}

