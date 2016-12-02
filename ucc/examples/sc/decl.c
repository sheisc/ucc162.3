/*************************************************************
 *		declaration
 *				int declarator

 *

 *

 *************************************************************/
#include <stdio.h>
#include "lex.h"
#include "expr.h"
#include "error.h"
#include "decl.h"
static AstNodePtr DirectDeclarator(void);
static AstNodePtr PostfixDeclarator(void);
static AstNodePtr Declarator(void);
////////////////////////////////////////////////////////////////
/********************************************************
   direct-declarator:
 		ID
 		( declarator )
 ********************************************************/
static AstNodePtr DirectDeclarator(void){
	AstNodePtr directDecl = NULL;
	if(curToken.kind == TK_ID){
		directDecl = CreateAstNode(TK_ID,&curToken.value,NULL,NULL);
		NEXT_TOKEN;		
	}else if(curToken.kind == TK_LPAREN){
		NEXT_TOKEN;
		directDecl = Declarator();
		Expect(TK_RPAREN);
	}else{
		Error("decl: id or '(' expected.\n");
	}
	return directDecl;
}
/*******************************************************
   postfix-declarator:
 		direct-declarator
 		postfix-declarator [num]
 		postfix-declarator (void)
 *******************************************************/
static AstNodePtr PostfixDeclarator(void){
	AstNodePtr decl = DirectDeclarator();
	while(1){
		if(curToken.kind == TK_LBRACKET){
			NEXT_TOKEN;
			decl = CreateAstNode(TK_ARRAY,&curToken.value,NULL,decl);			
			if(curToken.kind == TK_NUM){				
				decl->value.numVal = curToken.value.numVal;
				NEXT_TOKEN;
			}else{
				decl->value.numVal = 0;
			}
			Expect(TK_RBRACKET);
		}else if(curToken.kind == TK_LPAREN){
			AstNodePtr * param;
			NEXT_TOKEN;
			decl = CreateAstNode(TK_FUNCTION,&curToken.value,NULL,decl);
			param = &(decl->kids[0]);
			while(curToken.kind == TK_INT){				
				*param = CreateAstNode(TK_INT,&curToken.value,NULL,NULL);
				param = &((*param)->kids[0]);
				NEXT_TOKEN;
				if(curToken.kind == TK_COMMA){
					NEXT_TOKEN;
				}
			}			
			Expect(TK_RPAREN);
		}else{
			break;
		}
	}
	return decl;
}
/**************************************************
   	declarator:
 				* declarator
 				postfix-declarator
 **************************************************/
static AstNodePtr Declarator(void){
	if(curToken.kind == TK_MUL){
		AstNodePtr pointerDec;
		NEXT_TOKEN;
		pointerDec = CreateAstNode(TK_POINTER,&curToken.value,NULL,NULL);
		pointerDec->kids[1] = Declarator();
		return pointerDec;
	}
	return PostfixDeclarator();
}

AstNodePtr Declaration(void){
	AstNodePtr dec = NULL;
	if(curToken.kind == TK_INT){
		dec = CreateAstNode(TK_INT,&curToken.value,NULL,NULL);
		NEXT_TOKEN;
		dec->kids[1] = Declarator();
	}else{
		Expect(TK_INT);
	}
	return dec;
}

void VisitDeclarationNode(AstNodePtr pNode){
	AstNodePtr curParam;
	if(pNode){
		VisitDeclarationNode(pNode->kids[1]);
		switch(pNode->op){
		case TK_POINTER:
			printf("pointer to ");
			break;
		case TK_FUNCTION:
			printf("function(");
			curParam = pNode->kids[0];
			while(curParam){
				printf("%s",curParam->value.name);
				curParam = curParam->kids[0];
				if(curParam){
					printf(",");
				}
			}
			printf(") which returns ");
			break;
		case TK_ARRAY:
			printf("array[%d] of ",pNode->value.numVal);
			break;
		case TK_ID:
			printf("%s is:  ",pNode->value.name);
			break;
		case TK_INT:
			printf("int \n");
			break;
		default:
			printf("unknown: %s",pNode->value.name);
		}
	}
}

