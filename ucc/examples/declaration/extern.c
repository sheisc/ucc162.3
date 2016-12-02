#include <stdio.h>
		typedef struct {
		    char *Istack[20];
		} block_debug ;

		extern block_debug DEBUG;
		int a;
		void test(void){

			typedef struct {
				 char *Istack[20];
			} block_debug ;
			extern double a;
			extern block_debug DEBUG;

			int i;	
			for(i = 0; i < 20; i++){
				DEBUG.Istack[i-1]= NULL;
			}
		}
		int main(){
			extern int a = 30;
			return 0;
		}
