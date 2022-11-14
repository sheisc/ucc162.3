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

#define	IS_PREFIX_OF_DECL(tk)	(tk == TK_INT)

#ifdef _WIN32
#define	snprintf	_snprintf
#endif

int snprintf(char *str, size_t size, const char *format, ...);


static AstStmtNodePtr ExpressionStatement(void);
static AstStmtNodePtr IfStatement(void);
static AstStmtNodePtr WhileStatement(void);

static int isPrefixOfStatement(TokenKind tk);
static int NewLabel(void);
/////////////////////////////////////////////////////////////////////
static TokenKind prefixOfStmt[] = {
	TK_ID,TK_IF,TK_WHILE,TK_LBRACE,TK_INT,TK_INPUT,TK_OUTPUT, TK_RETURN
};



///////////////////////////////////////////////////////////////////////////////
static int NewLabel(void){
	static int labelNo;
	return labelNo++;
}


static AstNodePtr CreateLabelNode(void){
	Value value;
	memset(&value,0,sizeof(Value));
	snprintf(value.name,MAX_ID_LEN,"Label_%d",NewLabel());
	return CreateAstNode(TK_LABEL,&value,NULL,NULL);
}

AstStmtNodePtr CreateStmtNode(TokenKind op){
	AstStmtNodePtr pNode = (AstStmtNodePtr) malloc(sizeof(struct astStmtNode));
	memset(pNode,0,sizeof(*pNode));
	pNode->op = op;
	return pNode;
}

static int isPrefixOfStatement(TokenKind tk){
	int i = 0;
	for(i = 0; i < sizeof(prefixOfStmt)/sizeof(prefixOfStmt[0]); i++){
		if(tk == prefixOfStmt[i]){
			return 1;
		}
	}
	return 0;
}

// global variable declaration
static AstStmtNodePtr DeclStatement(void){
	if(IS_PREFIX_OF_DECL(curToken.kind)){
		// declaration ;
		AstStmtNodePtr decl = CreateStmtNode(TK_DECLARATION);
		decl->expr = Declaration();
		Expect(TK_SEMICOLON);
		return decl;		
	}else{
		Error("int  expected. Found in FIle %s  line %d\n",__FILE__,__LINE__);
		return NULL;
	}
}

// for global variables
AstStmtNodePtr Declarations(void){
	AstStmtNodePtr decls;
	AstStmtNodePtr * pStmt;
	Value value;
	
	decls = CreateStmtNode(TK_DECLS);
	pStmt = &(decls->next);
	while(IS_PREFIX_OF_DECL(curToken.kind)){
		*pStmt = DeclStatement();
		pStmt = &((*pStmt)->next);
	}
	
	return decls;
}

// see:  void EmitDeclarationNode(AstNodePtr pNode) in decl.c
static void DoLocalDeclarationNode(AstNodePtr pNode){
	if(pNode){
		DoLocalDeclarationNode(pNode->kids[1]);
		if(pNode->op == TK_ID){
			// FIXME: need to check whether there are local variables with the same name. Scope not supported yet.
			AstFuncDefNodePtr func = GetCurFuncDef();
			int cnt = func->local_vars_cnt;
	        memset(&func->local_vars[cnt], 0, sizeof(func->local_vars[cnt]));
			func->local_vars[cnt].op = TK_ID;
			func->local_vars[cnt].value = pNode->value;// curToken.value;
			// minus offset for local variables
			func->local_vars[cnt].offset = GetFrameOffset();
			func->local_vars_cnt++;
			// char buf[128];
			// sprintf(buf, "func->local_vars_cnt = %d %s\n", func->local_vars_cnt, pNode->value.name);
			// perror(buf);
		}
	}
}


static AstStmtNodePtr ExpressionStatement(void){
	if(curToken.kind == TK_ID){
		AstStmtNodePtr res_stmt = NULL;
		Token savedToken = curToken;
		NEXT_TOKEN;
		if(curToken.kind == TK_ASSIGN){	
			//	id = expression;
			AstStmtNodePtr assign = CreateStmtNode(TK_ASSIGN);
			assign->kids[0] = CreateAstNode(TK_ID, &savedToken.value,NULL,NULL);			
			NEXT_TOKEN;
			assign->expr = Expression();
			res_stmt = assign;
		}
		else if (curToken.kind == TK_LPAREN){ 
			// FUNCTION CALL
			// id (expr, expr, ...)
			AstStmtNodePtr call = CreateStmtNode(TK_CALL);
			call->expr = FunctionCallExpression(savedToken);
			res_stmt = call;
		}
		else{
			Error("stmt:	'=' expected.  Found in  File  %s  line  %d\n", __FILE__, __LINE__);
		}
		Expect(TK_SEMICOLON);
		return res_stmt;
	}
#if 1		// 	allow local variables.	
	else if(IS_PREFIX_OF_DECL(curToken.kind)){ // int var;
		// declaration ;
		AstStmtNodePtr decl = CreateStmtNode(TK_DECLARATION);
		decl->expr = Declaration();
		Expect(TK_SEMICOLON);
		DoLocalDeclarationNode(decl->expr);
		return decl;		
	}
#endif	
	else{
		Error("stmt:	id expected. Found in FIle %s  line %d\n",__FILE__,__LINE__);
		return NULL;
	}
}

/**************************************************
 (1)  if(e) {} else {}
	kids[0] 	label_false
	kids[1]		label_next

	if(!expr) goto label_false	
	...
	goto lable_next	
label_false:
	...
label_next:
	...

 (2)  if(e) {} 
	kids[0]		label_next

	if(!expr)	goto label_next
	...
label_next:
	...
 **************************************************/
static AstStmtNodePtr IfStatement(void){
	AstStmtNodePtr ifStmt = NULL;

	ifStmt = CreateStmtNode(TK_IF);
	Expect(TK_IF);
	Expect(TK_LPAREN);
	ifStmt->expr = Expression();
	Expect(TK_RPAREN);
	
	ifStmt->thenStmt = Statement();
	ifStmt->kids[0] = CreateLabelNode();
	if(curToken.kind == TK_ELSE){
		NEXT_TOKEN;
		ifStmt->elseStmt = Statement();
		// label for the statement after if-statment
		ifStmt->kids[1] = CreateLabelNode();
	}
	
	return ifStmt;
	
}

/**********************************************
	label_begin:
			if(...) goto	lable_next
			......
			goto label_begin
	lable_next
 **********************************************/
static AstStmtNodePtr WhileStatement(void){
	AstStmtNodePtr whileStmt = NULL;
	whileStmt = CreateStmtNode(TK_WHILE);
	
	whileStmt->kids[0] = CreateLabelNode();
	Expect(TK_WHILE);
	Expect(TK_LPAREN);
	whileStmt->expr = Expression();
	Expect(TK_RPAREN);
	whileStmt->thenStmt = Statement();
	// lable for the statement after while	
	whileStmt->kids[1] = CreateLabelNode();
	return whileStmt;
}

//	comStmt->next ----> list of statements
AstStmtNodePtr CompoundStatement(void){
	AstStmtNodePtr comStmt;
	AstStmtNodePtr * pStmt;
	Value value;

	comStmt = CreateStmtNode(TK_COMPOUND);
	pStmt = &(comStmt->next);

	Expect(TK_LBRACE);
	while(isPrefixOfStatement(curToken.kind)){
		*pStmt = Statement();
		pStmt = &((*pStmt)->next);
	}
	Expect(TK_RBRACE);
	return comStmt;
}

// input(id);
// output(id);
AstStmtNodePtr  IOStatement(TokenKind  tk){
	AstStmtNodePtr ioStmt;
	ioStmt =  CreateStmtNode(tk);
	Expect(tk);
	Expect(TK_LPAREN);
	if(curToken.kind == TK_ID){
		ioStmt->expr = CreateAstNode(curToken.kind,&curToken.value,NULL,NULL);		
		NEXT_TOKEN;		
	}else{
		Error("id expected.  Found in File %s line %d\n",__FILE__,__LINE__);
	}
	Expect(TK_RPAREN);
	Expect(TK_SEMICOLON);
	return ioStmt;
}

// return expr;
AstStmtNodePtr  ReturnStatement(){
	AstStmtNodePtr returnStmt = NULL;
	returnStmt = CreateStmtNode(TK_RETURN);
	Expect(TK_RETURN);
	returnStmt->expr = Expression();	
	// FIXME: allocate a temporary for the return value
	returnStmt->expr->offset = GetFrameOffset();
	Expect(TK_SEMICOLON);
	return returnStmt;
}


AstStmtNodePtr Statement(void){
	switch(curToken.kind){
	case TK_IF:
		return IfStatement();
	case TK_WHILE:
		return WhileStatement();
	case TK_LBRACE:
		return CompoundStatement();
	case TK_ID:
	case TK_INT:
		return ExpressionStatement();
	case TK_INPUT:
	case TK_OUTPUT:
		return IOStatement(curToken.kind);
	case TK_RETURN:
	 	return ReturnStatement();
	}
	
	assert(0);
	return NULL;
}



void   EmitStatementNode(AstStmtNodePtr stmt){
	if(!stmt){
		return;
	}
	switch(stmt->op){
	case TK_IF:
		/*****************************************************
			Lab 6
		******************************************************/		
		if(stmt->kids[0] && stmt->kids[1]){

			EmitArithmeticNode(stmt->expr);
			EmitAssembly("cmpl $0, %s",GetAccessName(stmt->expr));
			EmitAssembly("je %s", stmt->kids[0]->value.name);
			EmitStatementNode(stmt->thenStmt);
			EmitAssembly("jmp %s", stmt->kids[1]->value.name);
			EmitLabel("%s:", stmt->kids[0]->value.name);
			EmitStatementNode(stmt->elseStmt);
			EmitLabel("%s:", stmt->kids[1]->value.name);
		}else{

			EmitArithmeticNode(stmt->expr);
			EmitAssembly("cmpl $0, %s", GetAccessName(stmt->expr));
			EmitAssembly("je %s", stmt->kids[0]->value.name);
			EmitStatementNode(stmt->thenStmt);
			EmitLabel("%s:", stmt->kids[0]->value.name);
		}
		break;
	case TK_WHILE:
		/*****************************************************
			Lab6
		******************************************************/
		
		EmitLabel("%s:", stmt->kids[0]->value.name);
		EmitArithmeticNode(stmt->expr);
		EmitAssembly("cmpl $0, %s",GetAccessName(stmt->expr));
		EmitAssembly("je %s", stmt->kids[1]->value.name);
		EmitStatementNode(stmt->thenStmt);
		EmitAssembly("jmp %s", stmt->kids[0]->value.name);
		EmitLabel("%s:", stmt->kids[1]->value.name);

		break;
	case TK_COMPOUND:
		while(stmt->next){
			EmitStatementNode(stmt->next);
			stmt = stmt->next;
		}
		break;
	case TK_DECLS:  // global variables
		while(stmt->next){
			EmitDeclarationNode(stmt->next->expr);
			stmt = stmt->next;
		}
		break;
	case TK_ASSIGN:
		EmitArithmeticNode(stmt->expr);
		if(stmt->kids[0] && stmt->expr){
			EmitAssembly("movl %s, %%eax",GetAccessName(stmt->expr));
			EmitAssembly("movl %%eax, %s",GetAccessName(stmt->kids[0]));
		}
		break;
	case TK_INPUT:	//input(id);
		//printf("\tinput(%s) \n",stmt->expr->value.name);
		/****************************************************
		leal .input_fmtstr, %eax
		leal a, %ecx
		pushl %ecx
		pushl %eax
		call scanf
		addl $8, %esp
		*****************************************************/
		EmitAssembly("leal %s, %%eax",INPUT_FORMAT_STR_NAME);
		EmitAssembly("leal %s, %%ecx",GetAccessName(stmt->expr));
		EmitAssembly("pushl %%ecx");
		EmitAssembly("pushl %%eax");
		EmitAssembly("call scanf");
		EmitAssembly("addl $8, %%esp");
		break;
	case TK_OUTPUT:	//output(id);
		//printf("\toutput(%s) \n",stmt->expr->value.name);
		/*****************************************************
			leal .output_fmtstr, %eax
			pushl a
			pushl %eax
			call printf
			addl $8, %esp
 		 *****************************************************/
		EmitAssembly("leal %s, %%eax",OUTPUT_FORMAT_STR_NAME);
		EmitAssembly("pushl %s",GetAccessName(stmt->expr));
		EmitAssembly("pushl %%eax");
		EmitAssembly("call printf");
		EmitAssembly("addl $8, %%esp");
		break;
	case TK_CALL: 
		// FIXME:
		EmitArithmeticNode(stmt->expr);
		break;
	case TK_RETURN:
		// FIXME:
		EmitArithmeticNode(stmt->expr);
		EmitAssembly("movl %s, %%eax",GetAccessName(stmt->expr));
	}
}

