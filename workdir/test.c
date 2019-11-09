#include <stdio.h>

void foo(int a ){
	printf("got %d\n", a);
}

void test(int a,int b){
	while(a>0){
		if(b==0){printf("b<a\n");return;}
		a--;
		b--;
	}
	if(b==0){printf("b==a\n");return;}
	printf("b>a\n");
	return;
}

int main()
{
   int a,b;
   printf("Enter something:\n");
   scanf("%d", &a);
	 printf("Enter something else: \n");
   scanf("%d", &b);
	 if(a > 0 && a < 50){
		 if(b > 0 && b < 50){
			 foo(a);
			 foo(b);
			 test(a,b);
		 }
	 }

   return 0;
}
