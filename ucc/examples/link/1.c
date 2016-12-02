#include <stdio.h>
#include <math.h>
//float fabsf(float);
REAL_T add(REAL_T a , REAL_T b){
	return a+b;
}
extern int abc;
REAL_T fpVal = 20.0f;
/************************************************
	Undeclared fabs250()
		is treated as 	int fabs250() 	----without prototype
		in C Language
	(1) In UCC(see 1.s and 2.s)
		Before calling fabs250(...) in main(),
		we mov the address of "%x \n" to %eax,
		During the execution of fabs250(..),
		it happens to be that %eax is untouched.
		After fabs250(...), %eax is still the address
		of "%x \n".
	(2) In GCC(see 1.s and 2.s),
		the content of %eax is changed during fabs250(...).
		But because fabs250(...) is also considered as no-prototype,
		we do argument-promotion here,
		8.0f ---->  0x00000000	0x40200000.
		But when REAL_T is float,
		float fabs250(float val) only fetch the first 4 byes in 
		the stack ,that is , 0x00000000.
		So after fabs250(...), %eax is still 0.
 ************************************************/
int main(int argc,char *argv[]){
	double fp = fpVal;
	int * ptr = (int *) &fp;
	abc = 300;
	printf("%f: %08x %8x \n",fp,ptr[0],ptr[1]);
	printf("%x \n",fabs250(fpVal));
	printf("%f \n",add(fabs250(fpVal),3.0f));
	return 0;
}



