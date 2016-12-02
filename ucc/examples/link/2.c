#include <math.h>
//int abc = 200;
int abc;
int abc = 300;
REAL_T fabs250(REAL_T a){
#if 0
	REAL_T value =  (REAL_T)fabs(a);
	value += 1.0f;
	return value;
#endif
#if 1
	REAL_T value = fabs(a);
	return value;
#endif
}
