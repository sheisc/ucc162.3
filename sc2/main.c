#include <stdio.h>
#include "lex.h"
#include "expr.h"
#include "decl.h"
#include "stmt.h"
#include "emit.h"
#include "func.h"

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



static void output_library_func(char *setCmd, char *funcName){
	EmitLabel("\n.text");
	EmitLabel(".globl  %s", funcName);
	EmitLabel("%s:", funcName);
	EmitAssembly("movl 8(%%esp), %%eax");
	EmitAssembly("cmpl %%eax, 4(%%esp)");
	EmitAssembly("%s    %%al", setCmd);
	EmitAssembly("movzbl  %%al, %%eax");
	EmitAssembly("ret");
}

/* 
	int less(int a, int b);
	int larger(int a, int b);
	int equal(int a, int b);
*/
static void attach_our_library(void){
	/*
.text
.globl  SQAless
SQAless:
        movl    8(%esp), %eax
        cmpl    %eax, 4(%esp)
        setl    %al
        movzbl  %al, %eax
        ret


.text
.globl  SQAlarger
SQAlarger:
        movl    8(%esp), %eax
        cmpl    %eax, 4(%esp)
        setg    %al
        movzbl  %al, %eax
        ret

.text
.globl  SQAequal
SQAequal:
        movl    8(%esp), %eax
        cmpl    %eax, 4(%esp)
        sete    %al
        movzbl  %al, %eax
        ret

	*/
	output_library_func("setl", "SQAless");
	output_library_func("setg", "SQAlarger");
	output_library_func("sete", "SQAequal");

  /*

int SQAStore(char *base, int offset, int val){
    *((int *) (base + offset)) = val;
    return val;
}

SQAstore:
        movl    12(%esp), %eax
        movl    4(%esp), %ecx
        movl    8(%esp), %edx
        movl    %eax, (%ecx,%edx)
        ret
  */	
	EmitLabel("\n.text");
	EmitLabel(".globl  %s", "SQAstore");
	EmitLabel("%s:", "SQAstore");
	EmitAssembly("movl 12(%%esp), %%eax");
	EmitAssembly("movl 4(%%esp), %%ecx");
	EmitAssembly("movl 8(%%esp), %%edx");
	EmitAssembly("movl %%eax, (%%ecx,%%edx)");
	EmitAssembly("ret"); 

/*
int SQAload(char *base, int offset){
    return *((int *) (base + offset));
}

SQAload:
        movl    4(%esp), %edx
        movl    8(%esp), %eax
        movl    (%edx,%eax), %eax
        ret
*/
	EmitLabel("\n.text");
	EmitLabel(".globl  %s", "SQAload");
	EmitLabel("%s:", "SQAload");
	EmitAssembly("movl 4(%%esp), %%edx");
	EmitAssembly("movl 8(%%esp), %%eax");
	EmitAssembly("movl (%%edx,%%eax), %%eax");
	EmitAssembly("ret"); 
	
}

int main(){
	AstStmtNodePtr decls = NULL;
	AstFuncDefNodePtr funcs = NULL;
	
	InitLexer(NextCharFromStdin);
	NEXT_TOKEN;
	decls = Declarations();	
	funcs = FunctionDefinitions();
	Expect(TK_EOF);


	/*******************************************************
	.data
	.input_fmtstr:	.string	"%d"
	.output_fmtstr:	.string	"%d\012"
	********************************************************/
	EmitLabel("#Auto-Genereated by SE352");
	EmitLabel(".data");
	EmitAssembly("%s:	.string	\"%%d\"",INPUT_FORMAT_STR_NAME);
	EmitAssembly("%s:	.string	\"%%d\\012\"",OUTPUT_FORMAT_STR_NAME);
	EmitStatementNode(decls);
	/*********************************	
	.text
	.globl	main
	main:
	**********************************/
	attach_our_library();
	EmitFuncDefNode(funcs);
	// FIXME: TBD, free the heap memory.
	// To be simple, we only use malloc(), but never free() in this simple compiler.
	// Let the OS do it for us :)
	return 0;
}


