#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "c.h"
///////////////////////////////////////////////////////////////////////
static int  pc = 0;			//程序计数器
static int  bx = 0;			//变址寄存器 指向活动记录的开始位置

///////////////////////////////////////////////////////////////////////

static int getAddress(int offset);	//由活动记录内的偏移来求"变量的地址"

///////////////////////////////////////////////////////////////////////

//由偏移量求地址
static int getAddress(int offset){
	if(offset < 0){		//全局变量的地址
		return -offset;
	}
	else{				//局部变量的地址
		return bx+offset;
	}
}
void interpret(){
	int savedBx;
	int input, arg1,arg2,result ;
	while( pc != HaltingPC){
		switch(cs[pc].optr){
		case InsAddBx:
			bx += cs[pc].arg1;
			pc++;
			break;
		case InsSetBx:
			bx = cs[pc].arg1;
			pc++;
			break;
		case InsRet:
			savedBx = bx;
			bx -= ds[savedBx];			//因为存的是相对地址 所以要-=
			pc = ds[savedBx+1];
			break;
		case InsCall:
			pc = cs[pc].arg1;
			break;
		case InsUminus:
			arg1= getAddress(cs[pc].arg1);
			result = getAddress(cs[pc].result);
			ds[result] = -ds[arg1];
			pc++;
			break;
		case InsOut:
			arg1 = getAddress(cs[pc].arg1);
			printf("%d ",ds[arg1]);
			pc++;
			break;
		case InsIn:
			arg1 = getAddress(cs[pc].arg1);
			scanf("%d",&input);
			ds[arg1] = input;
			pc++;
			break;
		case InsNot:
		case InsAnd:
		case InsOr:
			printf("not supported yet");
			pc++;
			break;
		case InsJne:
			arg1 = getAddress(cs[pc].arg1);
			arg2 = getAddress(cs[pc].arg2);
			if(ds[arg1] != ds[arg2])
				pc = cs[pc].result;
			else
				pc++;
			break;
		case InsJeq:
			arg1 = getAddress(cs[pc].arg1);
			arg2 = getAddress(cs[pc].arg2);
			if(ds[arg1] == ds[arg2])
				pc = cs[pc].result;
			else
				pc++;
			break;
		case InsJge:
			arg1 = getAddress(cs[pc].arg1);
			arg2 = getAddress(cs[pc].arg2);
			if(ds[arg1] >= ds[arg2])
				pc = cs[pc].result;
			else
				pc++;
			break;
		case InsJgt:
			arg1 = getAddress(cs[pc].arg1);
			arg2 = getAddress(cs[pc].arg2);
			if(ds[arg1] > ds[arg2])
				pc = cs[pc].result;
			else
				pc++;
			break;
		case InsJle:
			arg1 = getAddress(cs[pc].arg1);
			arg2 = getAddress(cs[pc].arg2);
			if(ds[arg1] <= ds[arg2])
				pc = cs[pc].result;
			else
				pc++;
			break;
		case InsJlt:
			arg1 = getAddress(cs[pc].arg1);
			arg2 = getAddress(cs[pc].arg2);
			if(ds[arg1] < ds[arg2])
				pc = cs[pc].result;
			else
				pc++;
			break;
		case InsNop:
			pc++;
			break;
		case InsMod:
			arg1 = getAddress(cs[pc].arg1);
			arg2 = getAddress(cs[pc].arg2);
			result = getAddress(cs[pc].result);
			ds[result] = (ds[arg1]) % (ds[arg2]);
			pc++;
			break;
		case InsDiv:
			arg1 = getAddress(cs[pc].arg1);
			arg2 = getAddress(cs[pc].arg2);
			result = getAddress(cs[pc].result);
			ds[result] = ds[arg1]/ds[arg2];
			pc++;
			break;
		case InsMul:
			arg1 = getAddress(cs[pc].arg1);
			arg2 = getAddress(cs[pc].arg2);
			result = getAddress(cs[pc].result);
			ds[result] = ds[arg1]*ds[arg2];
			pc++;
			break;
		case InsSub:
			arg1 = getAddress(cs[pc].arg1);
			arg2 = getAddress(cs[pc].arg2);
			result = getAddress(cs[pc].result);
			ds[result] = ds[arg1]-ds[arg2];
			pc++;
			break;
		case InsAdd:
			arg1 = getAddress(cs[pc].arg1);
			arg2 = getAddress(cs[pc].arg2);
			result = getAddress(cs[pc].result);
			ds[result] = ds[arg1]+ds[arg2];
			pc++;
			break;
		case InsInit:
			arg1 = getAddress(cs[pc].arg1);
			ds[arg1] = cs[pc].arg2;
			pc++;
			break;
		case InsMov:
			arg1 = getAddress(cs[pc].arg1);
			arg2 = getAddress(cs[pc].arg2);
			ds[arg1] = ds[arg2];
			pc++;
			break;
		case InsJmp:
			pc = cs[pc].result;
			break;
		case InsJtrue:
			arg1 = getAddress(cs[pc].arg1);
			if(ds[arg1] != 0)
				pc = cs[pc].result;
			else 
				pc++;
			break;
		case InsJfalse:
			arg1 = getAddress(cs[pc].arg1);
			if(ds[arg1] == 0)
				pc = cs[pc].result;
			else 
				pc++;
			break;
		default:
			printf("unknown instruction.");
		}
	}
}
