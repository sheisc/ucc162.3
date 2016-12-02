#ifndef __UCC_H_
#define __UCC_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#include "input.h"
#include "error.h"
#include "alloc.h"
#include "vector.h"
#include "str.h"
#include "lex.h"
#include "type.h"
#include "symbol.h"


/***********************************************************************
 typedef struct{
	 int b;
	 struct{
		 //int d;
	 };
	 int c;
 }Data;

 ***********************************************************************/
#define ALIGN(size, align)  ((align == 0) ? size: ((size + align - 1) & (~(align - 1))))


extern Vector ExtraWhiteSpace;
extern Vector ExtraKeywords;
extern FILE  *ASTFile;
extern FILE  *IRFile;
extern FILE  *ASMFile;
extern char  *ExtName;
extern Heap CurrentHeap;
extern struct heap ProgramHeap;
extern struct heap FileHeap;
extern struct heap StringHeap;
extern int WarningCount;
extern int ErrorCount;


#include "gen.h"
extern char * ASMFileName;
char * GetUilCodeName(int code);
const char * GetAsmCodeName(int code);
#define	EMPTY_OBJECT_SIZE		1
#define	EMPTY_OBJECT_ALIGN		4
#define	IS_SPEACIAL_IRInst(inst)	(inst->opcode == RET  || inst->opcode == CALL || (inst->opcode >=JZ && inst->opcode <=IJMP))
#define	PRINT_CUR_IRInst(inst)	do{		\
									IRInst pInst = (IRInst) inst;		\
									printf("%s %d:  ", __FILE__,__LINE__);	\
									if(pInst && !IS_SPEACIAL_IRInst(pInst)){	\
										Coord pcoord = pInst->opds[0]->pcoord;		\
										if(pcoord){						\
											printf("%s %d ; ", pcoord->filename, pcoord->ppline);		\
										}							\
										if(pInst->opds[0] && pInst->opds[0]->name){				\
											printf(" %s   %s   ", pInst->opds[0]->name,GetUilCodeName(pInst->opcode) );		\
										}							\
										if(pInst->opds[1] && pInst->opds[1]->name){							\
											printf(" %s ", pInst->opds[1]->name);		\
										}		\
										if(pInst->opds[2] && pInst->opds[2]->name){				\
											printf(" %s ", pInst->opds[2]->name);		\
										}										\
									}								\
									printf("\n");		\
								}while(0)
								
#define	PRINT_CUR_ASTNODE(astNode)		do{		\
									printf("%s %d:  ", __FILE__,__LINE__);	\
									if(astNode){							\
										printf("%s %d  ", astNode->coord.filename, astNode->coord.ppline);		\
									}								\
									printf("\n");		\
								}while(0)

#define	PRINT_DEBUG_INFO(LP_args_RP)	do{printf("%s %d:  ", __FILE__,__LINE__);	\
									printf LP_args_RP ;	\
									printf("\n");	\
									}while(0)
#define	PRINT_I_AM_HERE()		PRINT_DEBUG_INFO(("\t 9527!\t I am here"))								



#endif


