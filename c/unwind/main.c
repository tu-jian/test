
/* Dummy function to avoid linker complaints */
void __aeabi_unwind_cpp_pr0(void)
{
}

void __aeabi_unwind_cpp_pr1(void)
{
}

char str[] = "abcdefg";

int sum(int a, int b)
{
	int c = a + a;
	int d = c + b;

	return c + d;
}

int main(void)
{
	int x = 1;
	int y = 2;
	int c;
	
	c = sum(x, y);
	
	for(c = 0; c < sizeof(str) / sizeof(str[0] - 1); c++)
	{
		str[c] = str[c] + 1;
	}
	
	return 0;
}
