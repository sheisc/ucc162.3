#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "lex.h"
#include "expr.h"
#include "error.h"

#ifdef _WIN32
#define	snprintf	_snprintf
#endif

int snprintf(char *str, size_t size, const char *format, ...);
/********************************************
	expr:
		id + id + id + ... + id
 ********************************************/

#define	ADD_RIGHT_ASSOCIATE	
#define	MUL_RIGHT_ASSOCIATE	

/////////////////////////////////////////////////////////////////////
static int NewTemp(void){
	static int tmpNo;
	return tmpNo++;
}

AstNodePtr CreateAstNode(TokenKind tk,Value * pVal,
		AstNodePtr left,AstNodePtr right){

	AstNodePtr pNode = (AstNodePtr) malloc(sizeof(struct astNode));
	// memset(pNode,0,sizeof(*pNode));
	pNode->op = tk;
	pNode->value = *pVal;
	pNode->kids[0] = left;
	pNode->kids[1] = right;
	return pNode;
}


//
void Expect(TokenKind tk){
	if(curToken.kind == tk){
		NEXT_TOKEN;
	}else{
		Error("%s expected.\n",GetTokenName(tk));		
	}
}
// 

static AstNodePtr PrimaryExpression(void){
	AstNodePtr expr = NULL;
	if(curToken.kind == TK_ID || curToken.kind == TK_NUM){
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
void VisitArithmeticNode(AstNodePtr pNode){
	if(pNode && IsArithmeticNode(pNode)){
		VisitArithmeticNode(pNode->kids[0]);
		VisitArithmeticNode(pNode->kids[1]);
		if(pNode->kids[0] && pNode->kids[1]){
			printf("\t%s = ",	pNode->value.name);
			Do_PrintNode(pNode->kids[0]);
			printf("%s ",GetTokenName(pNode->op));
			Do_PrintNode(pNode->kids[1]);
			printf("\n");
		}
	}
}

