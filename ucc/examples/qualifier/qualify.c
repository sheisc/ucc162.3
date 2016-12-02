#if 1
#include <stdio.h>
struct Data{
	int arr[4];
};
/*****************************************************
	 cause fatal error , 
		see (1)type.c Qualify()
			 (2) declchk.c	CheckInitializerInternal()	,if(categ == STRUCT)
 *****************************************************/
const struct Data dt = {
	1,2,3,4
};
int main(){

	return 0;
}
#endif


#if 0
#include <stdio.h>
/********************************************************
//
int IsIncompleteRecord(Type ty){
	ty = Unqual(ty);
	return ( IsRecordType(ty) && !((RecordType) ty)->complete);
}
 ********************************************************/
#include <stdio.h>
typedef struct{
	int arr[4];
}Data;

void f(const Data * ptr);
int main(){

	return 0;
}


#endif
