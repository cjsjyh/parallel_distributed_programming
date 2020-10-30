#include <stdio.h>

#define MAX(a,b) (((a)>(b))?(a):(b))

int main(){
  int a, b, c, d, temp;
  a= 10;
  b= 1;
  c= -10;
  d= 23;

  temp = MAX(a,b);
  printf("%d\n", temp);
  temp = MAX(temp, c);
  printf("%d\n", temp);
  temp = MAX(temp, d);
  printf("%d\n", temp);
  return 0;
}
