#include <stdio.h>

#define	DEF_VARS(IntType,a,va,b,vb,c,vc)		IntType a = va, b = vb,c = vc

#define	TEST_ADD_MUL_OP(a,b,c,format)		c = a + b;	printf(format,c);\
											c = a - b;	printf(format,c);\
											c = a * b;	printf(format,c);\
											c = a / b;	printf(format,c);\
											c++;			printf(format,c);\
											c--;			printf(format,c--);

#define	TEST_ARITH_OP(a,b,c,format)		c = a|b;	printf(format,c);\
											c = a&b;	printf(format,c);\
											c = a^b;	printf(format,c);\
											c = a << 2;	printf(format,c);\
											c = a >> 2;	printf(format,c);\
											TEST_ADD_MUL_OP(a,b,c,format);\
											c = a % b;	printf(format,c);\
											c = -a;	printf(format,c);\
											c = ~a;	printf(format,c);

DEF_VARS(int,sa,1,sb,2,sc,3);
DEF_VARS(unsigned int,ua,1,ub,2,uc,3);
DEF_VARS(float,fa,1.0f,fb,2.0f,fc,3.0f);
DEF_VARS(double,da,1.0,db,2.0,dc,3.0);
void DoTest(void){
	TEST_ARITH_OP(sa,sb,sc,"sc = %x \n");
	TEST_ARITH_OP(ua,ub,uc,"uc = %x \n");	
	TEST_ADD_MUL_OP(fa,fb,fc,"fc = %f \n");
	TEST_ADD_MUL_OP(da,db,dc,"dc = %f \n");
}

int main(){
	DoTest();
	return 0;
}



