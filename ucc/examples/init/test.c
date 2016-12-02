#include <stdio.h>
struct { int a[3], b; } w[] = { { 1 }, 2 };

         int q1[4][3][2] = {
                  { 1 },
                  { 2, 3 },
                  { 4, 5, 6 }
         };

         int q2[4][3][2] = {
                  1, 0, 0, 0, 0, 0,
                  2, 3, 0, 0, 0, 0,
                  4, 5, 6
         };

         int q3[4][3][2] = {
                  {
                           { 1 },
                  },
                  {
                           { 2, 3 },
                  },
                  {
                           { 4, 5 },
                           { 6 },
                  }
         };

			int x[] = { 1, 3, 5 };			
         int y1[4][3] = {
                  { 1, 3, 5 },
                  { 2, 4, 6 },
                  { 3, 5, 7 },
         };
         int y2[4][3] = {
                  1, 3, 5, 2, 4, 6, 3, 5, 7
         };
         int z[4][3] = {
                  { 1 }, { 2 }, { 3 }, { 4 }
         };
void printInts(int * ptr,int len){
	int i = 0;
	for(i = 0; i < len; i++){
		printf("%d ",ptr[i]);
	}
	printf("\n");
}


int main(int argc,char * argv[]){
	printInts(&x[0],3);
	printInts(&y1[0][0],4*3);
	printInts(&y2[0][0],4*3);
	printInts(&z[0][0],4*3);
	printInts((int*) &w[0],4);
	printInts((int*) &w[1],4);
	printInts((int*) &q1[0][0][0],4*3*2);
	printInts((int*) &q2[0][0][0],4*3*2);
	printInts((int*) &q3[0][0][0],4*3*2);
	return 0;
}

