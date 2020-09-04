#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "c.h"

#define ZCW_DEBUG

//////////////////////////////////////////////////////////////////////
int errorCount = 0;
int ds[DsSize];					//数据段	0号单元不用		
Instruction cs[CsSize];			//代码段
int csIndex = 2;		//代码段下标  1号预留给(最终会）跳转到main的指令
						//0号单元用于存放初始"变址寄存器"的指令
//int dsIndex = 0;		//数据段下标
FILE * infile;		//输入文件
FILE * outfile;		//输出文件


char * instrs[] = {
	"Jtrue",		//为真则跳转	(InsJtrue, arg1, , dest)
	"Jfalse",	//为假则跳转	(InsJfalse,arg1, , dest)
	"Jmp",		//无条件跳转	(InsJmp,  ,   , dest)	
	"Mov",		//数据复制      (InsMov, arg1,	  ,dest)
	"Init",		//初始化某单元	(InsInit,arg1,num,	   )
	"Add",		//加法			(InsAdd, arg1,arg2,dest)
	"Sub",		//减法			(InsSub, arg1,arg2,dest)
	"Mul",		//乘法			(InsMul, arg1,arg2,dest)
	"Div",		//除法			(InsDiv, arg1,arg2,dest)
	"Mod",		//取余
	"Nop",		//空操作		(InsNop,	,	, 	)
	"Jlt",		//判断是否<		(InsLt,arg1,arg2,result)
	"Jle",		//判断是否<=	(InsLe,arg1,arg2,result)
	"Jgt",		//判断是否>		(InsGt,arg1,arg2,result)
	"Jge",		//判断是否>=	
	"Jeq",		//判断是否==
	"Jne",		//判断是否!=
	"Or",		//逻辑或运算
	"And",		//逻辑与运算
	"Not",		//逻辑非运算
	"In",		//输入
	"Out",		//输出
	"Uminus",	//求相反数
	"Call",		//过程调用		(InsCall,des, , ,);		
	"Ret",		//过程返回		(InsRet,expr, , );
	"SetBx",	//设置bx指针，指向活动记录首地址(InsSetBx,addr, , )
	"AddBx",	//增加bx的值
};
//////////////////////////////////////////////////////////////////////////

void printCsInfo(){	
	for(int i = 0; i < csIndex; i++){
		/*
		printf("%s %d %d %d\n",
			instrs[cs[i].optr],cs[i].arg1,cs[i].arg2,cs[i].result);
			*/
		fprintf(outfile,"%d:\t %s %d %d %d\n",
			i,instrs[cs[i].optr],cs[i].arg1,cs[i].arg2,cs[i].result);
	}
}



/**
* 产生一条四元式指令
*/
void gen(int instrType, int arg1,int arg2,int result){
	if(csIndex >= CsSize){
		printf("Code Segment overflows.");
		exit(1);
	}
	//
	cs[csIndex].optr = instrType;
	cs[csIndex].arg1 = arg1;
	cs[csIndex].arg2 = arg2;
	cs[csIndex].result = result;
	csIndex++;
}
/**
* 语法错误处理
*/
void syntaxError(char * info){
	errorCount++;
	printf("line %d : %s\n",lineNumber,info);
	//naive error recovery strategy,but simple :)
	//added by zcw, 2010.2.25
	exit(1);
}

/**
* main函数
*/
int main(int argc,char * args[]){
	
	if(argc >= 2){	
		if((infile = fopen(args[1],"r")) == NULL){
			printf("can't open file %s .\n",args[1]);
			exit(1);
		}
		else{
			char name[80];
			strcpy(name,args[1]);
			char * dot = strchr(name,'.');
			if(dot == NULL){
				strcat(name,".s");
			}
			else{
				strcpy(dot,".s");
			}
			if( (outfile = fopen(name,"w")) == NULL){
				fclose(infile);
				printf("can't not create file %s.\n",name);
				exit(1);
			}			
		}
	}
	else{
		printf("parameter specifying input file needed.\n");
		exit(1);
	}	
	getToken();
	programBody();	
	if(token != EOF){		
		syntaxError("EOF needed.");	
	}
	printCsInfo();
	if(errorCount == 0)
		interpret();
	else
		printf("there are %d errors in program.",errorCount);
	fclose(infile);
	fclose(outfile);
	printf("\n");	
	return 0;
}
