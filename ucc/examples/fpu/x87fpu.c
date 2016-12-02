#include <stdio.h>


#if 0
#include <stdio.h>
#include <math.h>
double add(double a , double b){
	return a+b;
}
int main(){
	float f = 1.23f;
	float f1 = f+1.23f;
	printf("%f \n",add(fabs(f+1.23f)+100.0f,1.23f+f1+100.0f));
	return 0;
}
#endif


#if 0
				double c = 9.87;
				float d;
				unsigned int a = 0xFFFFFFFF;
				int b = 0xFFFFFFFF;
				int main(){
					
				
					c = (double) a;
					d = (float)a;
					printf("%f %f \n",c,d);

					a = (unsigned int) c;
					b = (int) c;
					printf("%x %d \n",a,b);

					a = (unsigned int) d;
					b = (int) d;
					printf("%x %d \n",a,b);
					printf(".....................................\n");
					c = (double) b;
					d = (float)b;
					printf("%f %f \n",c,d);

					a = (unsigned int) c;
					b = (int) c;
					printf("%x %d \n",a,b);

					a = (unsigned int) d;
					b = (int) d;
					printf("%x %d \n",a,b);
					return 0;
				}
#endif



#if 0
				double c = 9.87;
				float d;
				unsigned int a = 0xFFFFFFFF;
				int b = 0xFFFFFFFF;
				int main(){
					
				
					c = (double) a;
					d = (float)a;
					printf("%f %f \n",c,d);

					c = (double) b;
					d = (float)b;
					printf("%f %f \n",c,d);
					return 0;
				}
#endif

#if 0
 double c = 9.87;
 double d;
 int a = 50;char b = 'x';
 
 int main(){
	 a = (int)c;
	 printf("%d \n",a);
	 a = (int)(c+1.23);
	 b = (char)(c+1.23);
	 printf("%d %d .\n",a,b);
	 d = c + 11.23;
	 c = (double)((float)(c+11.23))+(c+1.23);
	 printf("%f\n",c);
	 return 0;
 }
#endif

#if 0
		 		double c = 9.87;
				double d;
				int main(){
					d = c + 1.23;
					c = (double)((float)(c+1.23))+(c+1.23);
					printf("%f\n",c);	
					return 0;
				}
#endif


#if 0
char sc = '\0';
unsigned char uc = '\0';
short ss = 0;
unsigned short us = 0;
int si = 0;
unsigned ui = 0;
float f32 = 0.0f;
double f64 = 0.0;
int main(){

	sc--; uc--; ss--; us--; si--; ui--; f32--; f64--;
	printf("%c %c %d %d %d %x %f %f\n",sc,uc,ss,us,si,ui,f32,f64);
	sc++; uc++; ss++; us++; si++; ui++; f32++; f64++;
	printf("%c %c %d %d %d %x %f %f\n",sc,uc,ss,us,si,ui,f32,f64);
	sc++; uc++; ss++; us++; si++; ui++; f32++; f64++;
	printf("%c %c %d %d %d %x %f %f\n",sc,uc,ss,us,si,ui,f32,f64);
	return 0;
}
#endif


#if 0
float GetValue(void){
	float f1 = 1.23f, f2;
	f2 = f1 + 1.0f;
	f2 = f1 + 2.0f;
	f2 = f1 + 3.0f;
	f2 = f1 + 4.0f;
	f2 = f1 + 5.0f;
	f2 = f1 + 6.0f;
	f2 = f1 + 7.0f;
	f2 = f1 + 8.0f;
	f2 = f1 + 9.0f;
	f2 = f1 + 10.0f;
	return 1.23f;
}
int main(){
	int i ;
	for(i = 0; i < 10; i++){
		GetValue();
	}
	for(i = 0; i < 10; i++){
		printf("%f \n",GetValue());
	}
	printf("%f \n",1.23f);
	return 0;
}
#endif


#if 0
			void f(void){
				float f1 = 1.0f, f2;
				f2 = f1+1.0f;
				f2 = (f1+1.0f)+(f1+1.0f);
				printf("%f \n",f2);
			}
			int main(){
				f();
				return 0;
			}
#endif


