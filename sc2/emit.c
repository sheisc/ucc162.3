#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "emit.h"


void EmitPrologue(int stksize){
	/************************
	pushl %ebp
	pushl %ebx
	pushl %esi
	pushl %edi
	movl %esp, %ebp
	subl $12, %esp
	**************************/
	EmitAssembly("pushl %%ebp");
	EmitAssembly("pushl %%ebx");
	EmitAssembly("pushl %%esi");
	EmitAssembly("pushl %%edi");
	EmitAssembly("movl %%esp, %%ebp");
	if (stksize != 0){
		EmitAssembly("subl $%d, %%esp",stksize);
	}
}

void EmitEpilogue(void){
	/********************************
	//movl $0, %eax
	------------------
	movl %ebp, %esp
	popl %edi
	popl %esi
	popl %ebx
	popl %ebp
	ret
	*********************************/
	//EmitAssembly("movl $0, %%eax");
	//---------------------------------
	EmitAssembly("movl %%ebp, %%esp");
	EmitAssembly("popl %%edi");
	EmitAssembly("popl %%esi");
	EmitAssembly("popl %%ebx");
	EmitAssembly("popl %%ebp");
	EmitAssembly("ret");
}

void EmitAssembly(const char * fmt, ...){
	va_list ap;
	va_start(ap, fmt);
	fprintf(stdout, "\t");
	vfprintf(stdout, fmt, ap);	
	fprintf(stdout, "\n");	
	va_end(ap);

}

void EmitLabel(const char * fmt, ...){
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stdout, fmt, ap);	
	fprintf(stdout, "\n");	
	va_end(ap);

}

