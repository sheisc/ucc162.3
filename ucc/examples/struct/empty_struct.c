/*************************************************
	assertion fails:
		see	
		Field AddField(Type ty, char *id, Type fty, int bits)
		see
	 void CheckStructDeclarator(Type rty, AstStructDeclarator stDec, Type fty)
 ************************************************/

#if 1
#include <stdio.h>
typedef struct{
	int b;
	union{
	};
	struct{
	int d;
	int c[];
	} a;
}Data;

int main(){
	//int arr[];
	Data d;
	printf("%d %d\n",sizeof(d.a.d),sizeof(d));
	return 0;
}
#endif

#if 0

#include <stdio.h>

         typedef struct tnode TNODE;	//  an uncomplete type here.
         struct tnode {
                  int count;
                  TNODE *left, *right;
         };
         TNODE s, *sp;

typedef struct{
	int b;
	struct{
		//int d;
	}a;
	int c;
}Data;

int main(){
	Data d1,d2;
	d1.b = 20;
	d1.c = 50;
	d1.a = d2.a;
	d1.c = 30;
	printf("%d \n",sizeof(d1));
	printf("%d %d \n",sizeof(d1),sizeof(d2.a));
	printf("%d %d \n",d1.b,d1.c);

	return 0;
}
#endif
