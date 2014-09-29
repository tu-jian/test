#include <stdio.h>
#include <stdarg.h>
#include <string.h>

int sub (int a, int b, char *pc)
{
	int aa = a + b;
	int bb = a - b;
	int i = 10;

	for (i = 0; i< 10; i++)
		aa = aa + strlen(pc);
	
	return aa - bb;
}

int add (int n, ...)
{
	va_list args;
	int temp, i, sum = 0;
	char *pc = "1234";
	
	va_start(args, n);

	for (i = 0; i < n; i++) {
		temp = va_arg(args, int);
		sum = sum + temp;		
	}

	sum = sub(sum, i, pc);

	va_end(args);

	return sum;
}

int test(int a, int b)
{
	int aa = a + 1;
	int bb = b + 1;
	int sum;

	sum = add(4, aa, bb, 3, 4);
	return sum;
}


int main ()
{
	int sum;

	sum = test(0, 1);
	printf("sum = %d\n", sum);

	return 0;
}
