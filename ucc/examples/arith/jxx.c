#include <stdio.h>

#define	DEF_VARS(IntType,a,va,b,vb,c,vc)		IntType a = va, b = vb,c = vc

int result ;
#if 1

#define	DO_TEST_COMPARE(condition)		if(condition){	\
														result = 1;	\
														printf("%s is true.\n",#condition);	\
													}else{	\
														result = 0;	\
														printf("%s is false.\n",#condition);	\
													}
#endif

#if 0
#define	TEST_COMPARE(a,b,c)		DO_TEST_COMPARE(a > b)
#endif
#if 1
#define	TEST_COMPARE(a,b,c)		DO_TEST_COMPARE(a)	\
											DO_TEST_COMPARE(!a)	\
											DO_TEST_COMPARE(a == b)	\
											DO_TEST_COMPARE(a != b)	\
											DO_TEST_COMPARE(a > b)	\
											DO_TEST_COMPARE(a < b)	\
											DO_TEST_COMPARE(a >= b)	\
											DO_TEST_COMPARE(a <= b)	
#endif

DEF_VARS(int,sa,1,sb,2,sc,3);
DEF_VARS(unsigned int,ua,1,ub,2,uc,3);
DEF_VARS(float,fa,2.0f,fb,2.0f,fc,3.0f);
DEF_VARS(double,da,2.0,db,2.0,dc,3.0);

void DoTest(void){
	TEST_COMPARE(sa,sb,sc);
	TEST_COMPARE(ua,ub,uc);
	TEST_COMPARE(fa,fb,fc);
	TEST_COMPARE(da,db,dc);
}

int main(){
	DoTest();	
	return 0;
}



