#include <stdio.h>
#include <stdlib.h>

int main()
{
	char *p;

	dmalloc_debug_setup("debug=0x4000503,log=/data/dmalloc.log");

	p = malloc(10);
	p[10] = p[0] + p[1];

	return p[10];
}
