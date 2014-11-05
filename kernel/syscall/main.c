#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

extern int my_getpid();

int main ()
{
	int pid;

	pid = getpid();
	printf("pid: %d\n", pid);

	pid = 0;
	pid = my_getpid();
	printf("my_getpid: %d\n", pid);

	return 0;
}
