#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "lex.h"
#include "expr.h"
#include "error.h"
#include "emit.h"
#include "func.h"

#ifdef _WIN32
#define	snprintf	_snprintf
#endif

int snprintf(char *str, size_t size, const char *format, ...);
/********************************************
	expr:
		id + id + id + ... + id
 ********************************************/

//#define	ADD_RIGHT_ASSOCIATE	
//#define	MUL_RIGHT_ASSOCIATE	



/////////////////////////////////////////////////////////////////////

static int offset;

static TokenKind prefixOfExpr[] = {
	TK_ID, TK_LPAREN, TK_NUM
};
/////////////////////////////////////////////////////////////////////

static int isPrefixOfExpression(TokenKind tk){
	int i = 0;
	for(i = 0; i < sizeof(prefixOfExpr)/sizeof(prefixOfExpr[0]); i++){
		if(tk == prefixOfExpr[i]){
			return 1;
		}
	}
	return 0;
}

int GetFrameSize(void){
	return -offset;
}

void ResetFrameSize(void){
	offset = 0;
}

static int NewTemp(void){
	static int tmpNo;
	return tmpNo++;
}

int GetFrameOffset(void){	
	offset -= 4;
	return offset;
}

AstNodePtr CreateAstNode(TokenKind tk,Value * pVal,
		AstNodePtr left,AstNodePtr right){

	AstNodePtr pNode = (AstNodePtr) malloc(sizeof(struct astNode));
	memset(pNode,0,sizeof(*pNode));
	pNode->op = tk;
	pNode->value = *pVal;
	pNode->kids[0] = left;
	pNode->kids[1] = right;
	return pNode;
}


//
void DoExpect(int line, char * fileName, TokenKind  tk){
	if(curToken.kind == tk){
		NEXT_TOKEN;
	}else{
		Error("Found in  File %s Line %d:  %s expected.\n",fileName, line, GetTokenName(tk));		
	}
}
// 

AstNodePtr FunctionCallExpression(Token savedToken){
	AstNodePtr expr = NULL;
	
	assert(savedToken.kind == TK_ID);
	assert(curToken.kind == TK_LPAREN);

	expr = CreateAstNode(TK_CALL,&savedToken.value,NULL,NULL);
	// allocate a temp for storing the return value
	expr->offset = GetFrameOffset();

	NEXT_TOKEN;
	// function call:  ID (expr, expr, ..., expr)
	int arg_cnt = 0;
	while(isPrefixOfExpression(curToken.kind)){
		expr->args[arg_cnt] = Expression();
		arg_cnt++;
		if(arg_cnt > MAX_ARG_CNT){
			Error("Only %d arguments allowed.\n", MAX_ARG_CNT);
		}
		if(curToken.kind == TK_COMMA){
			NEXT_TOKEN;
		} else if(curToken.kind == TK_RPAREN){			
			break;
		}
		else{
			Error("Illegal tokens in function call.\n");
		}

	}
	Expect(TK_RPAREN);
	expr->arg_cnt = arg_cnt;
	return expr;

}


static AstNodePtr PrimaryExpression(void){
	AstNodePtr expr = NULL;
	if(curToken.kind == TK_ID){
		Token savedToken = curToken;
		NEXT_TOKEN;
		if(curToken.kind == TK_LPAREN){ // function call
			expr = FunctionCallExpression(savedToken);
		}else{ // id
			expr = CreateAstNode(savedToken.kind,&savedToken.value,NULL,NULL);	
		}				
	}
	else if(curToken.kind == TK_NUM){
		expr = CreateAstNode(curToken.kind,&curToken.value,NULL,NULL);
		
		NEXT_TOKEN;		
	}else if(curToken.kind == TK_LPAREN){
		NEXT_TOKEN;
		expr = Expression();
		Expect(TK_RPAREN);		
	}else{
		Error("expr: id or '(' expected.\n");
	}
	return expr;
}


//	id - id - id -....
static AstNodePtr MultiplicativeExpression(void){
#ifndef	MUL_RIGHT_ASSOCIATE
	AstNodePtr left;
	left = PrimaryExpression();
	while(curToken.kind == TK_MUL || curToken.kind == TK_DIV){
		Value value;
		AstNodePtr expr;
		memset(&value,0,sizeof(value));
		snprintf(value.name,MAX_ID_LEN,"t%d",NewTemp());
		expr = CreateAstNode(curToken.kind,&value,NULL,NULL);
		expr->offset = GetFrameOffset();
		NEXT_TOKEN;
		expr->kids[0] = left;
		expr->kids[1] = PrimaryExpression();
		left = expr;
	}
	return left;
#else
	AstNodePtr left;
	left = PrimaryExpression();;
	if(curToken.kind == TK_MUL || curToken.kind == TK_DIV){
		Value value;
		AstNodePtr expr;
		memset(&value,0,sizeof(value));
		snprintf(value.name,MAX_ID_LEN,"t%d",NewTemp());
		expr = CreateAstNode(curToken.kind,&value,NULL,NULL);
		expr->offset = GetFrameOffset();
		NEXT_TOKEN;
		expr->kids[0] = left;
		expr->kids[1] = MultiplicativeExpression();
		return expr;
	}else{
		return left;
	}
#endif
}

static AstNodePtr AdditiveExpression(void){
#ifndef	ADD_RIGHT_ASSOCIATE
	AstNodePtr left;
	left = MultiplicativeExpression();
	while(curToken.kind == TK_SUB || curToken.kind == TK_ADD){
		Value value;
		AstNodePtr expr;
		memset(&value,0,sizeof(value));
		snprintf(value.name,MAX_ID_LEN,"t%d",NewTemp());
		expr = CreateAstNode(curToken.kind,&value,NULL,NULL);
		expr->offset = GetFrameOffset();
		NEXT_TOKEN;
		expr->kids[0] = left;
		expr->kids[1] = MultiplicativeExpression();
		left = expr;
	}
	return left;
#else
	AstNodePtr left;
	left = MultiplicativeExpression();
	if(curToken.kind == TK_SUB || curToken.kind == TK_ADD){
		Value value;
		AstNodePtr expr;
		memset(&value,0,sizeof(value));
		snprintf(value.name,MAX_ID_LEN,"t%d",NewTemp());
		expr = CreateAstNode(curToken.kind,&value,NULL,NULL);
		expr->offset = GetFrameOffset();
		NEXT_TOKEN;
		expr->kids[0] = left;
		expr->kids[1] = AdditiveExpression();
		return expr;
	}else{
		return left;
	}	
#endif
}

AstNodePtr Expression(void){
	return AdditiveExpression();
}

static int IsArithmeticNode(AstNodePtr pNode){
	return pNode->op == TK_SUB || pNode->op == TK_ADD 
				|| pNode->op == TK_MUL || pNode->op == TK_DIV;
}

static void Do_PrintNode(AstNodePtr pNode){
	if(pNode->op == TK_NUM){
		printf("%d ",pNode->value.numVal);
	}else{
		printf("%s ",pNode->value.name);
	}
}

static int Do_GetLocalVarOrParameterOffset(struct astNode *arr, int size, char *id){
	for(int i = 0; i < size; i++){
		int len = strlen(id);
		int len2 = strlen(arr[i].value.name);		
		if(strncmp(arr[i].value.name, id, len) == 0 && len == len2){ // equal
			return arr[i].offset;
		}
	}
	return 0;
}


static int GetLocalVarOrParameterOffset(char *id){
	AstFuncDefNodePtr func = GetCurFuncDef();
	assert(func);
	// check function formal parameters first
	int offset = Do_GetLocalVarOrParameterOffset(func->paras, func->para_cnt, id);
	if(offset == 0){
		// then check local variables
		offset = Do_GetLocalVarOrParameterOffset(func->local_vars, func->local_vars_cnt, id);
	}
	return offset;
}

char  *  GetAccessName(AstNodePtr pNode){
	if(pNode){
		if(pNode->op == TK_ID){ 
			int offset = GetLocalVarOrParameterOffset(pNode->value.name);
			if(offset != 0){ // FIXME:  PARAS: offset > 0;  local variables: offset < 0
				snprintf(pNode->accessName, MAX_ID_LEN+1, "%d(%%ebp)", offset);
			}
			else { // global variables, sum, oddSum, a,b,i
				snprintf(pNode->accessName, MAX_ID_LEN+1, "%s", pNode->value.name);
			}
		}else if(pNode->op == TK_NUM){ // 20,15
			snprintf(pNode->accessName, MAX_ID_LEN+1, "$%d", pNode->value.numVal);
		}else{	// temporary variables, a+b
			snprintf(pNode->accessName, MAX_ID_LEN+1, "%d(%%ebp)", pNode->offset);
		}
		return pNode->accessName;
	}else{
		return "";
	}
}

void EmitArithmeticNode(AstNodePtr pNode){ // function call is also handled here
	if(pNode && pNode->op == TK_CALL){
		int cnt = 0;
		// evaluate argugments from left to right	
		while (cnt < pNode->arg_cnt){
			EmitArithmeticNode(pNode->args[cnt]);			
			cnt++;
		}
		// push arguments from right to left
		cnt = pNode->arg_cnt - 1;		
		while (cnt >= 0){
			EmitAssembly("pushl %s",GetAccessName(pNode->args[cnt]));
			cnt--;
		}
		// call the function 
		EmitAssembly("call %s", pNode->value.name);
		if(pNode->arg_cnt > 0) {
			EmitAssembly("addl $%d, %%esp", CPU_WORD_SIZE_IN_BYTES * pNode->arg_cnt);
		}
		// move the return address to a temporary variable
		EmitAssembly("movl %%eax, %s",GetAccessName(pNode));
	}
	else if(pNode && IsArithmeticNode(pNode)){
		EmitArithmeticNode(pNode->kids[0]);
		EmitArithmeticNode(pNode->kids[1]);
		if(pNode->kids[0] && pNode->kids[1]){  
			switch(pNode->op){
			case TK_ADD:
				EmitAssembly("movl %s, %%eax",GetAccessName(pNode->kids[0]));
				EmitAssembly("addl %s, %%eax",GetAccessName(pNode->kids[1]));
				EmitAssembly("movl %%eax, %s",GetAccessName(pNode));
				break;
			case TK_SUB:
				/*********************************
				Lab5
				*********************************/
				EmitAssembly("movl %s, %%eax",GetAccessName(pNode->kids[0]));
				EmitAssembly("subl %s, %%eax",GetAccessName(pNode->kids[1]));
				EmitAssembly("movl %%eax, %s",GetAccessName(pNode));
				break;
			case TK_MUL:
				/*********************************
				Lab5
				*********************************/
				EmitAssembly("movl %s, %%eax",GetAccessName(pNode->kids[0]));
				EmitAssembly("imull %s, %%eax",GetAccessName(pNode->kids[1]));
				EmitAssembly("movl %%eax, %s",GetAccessName(pNode));
				break;
			case TK_DIV:
				/*********************************
				Lab5
					movl  a, %eax
					cdq
					movl  b, %ebx
					idivl  %ebx
					movl  %eax, -16(%ebp)
				*********************************/	
				EmitAssembly("movl %s, %%eax",GetAccessName(pNode->kids[0]));
				EmitAssembly("cdq");
				EmitAssembly("movl %s, %%ebx",GetAccessName(pNode->kids[1]));
				EmitAssembly("idivl %%ebx");
				EmitAssembly("movl %%eax, %s",GetAccessName(pNode));
				break;
			}
		}
	}
}
