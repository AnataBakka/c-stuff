//a very bad sqrt approximation for finding the ipotenuse of a triangle
#include <stdio.h>

float sqrt2(float x, float y){
	return x*(1.074074074074)+(float)y*y/(3*x);
}

void main(int argc,char**argv){
	printf("%f",sqrt2(12,18));
}
