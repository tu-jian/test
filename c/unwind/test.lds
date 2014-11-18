OUTPUT_ARCH(arm)
ENTRY(_start)

SECTIONS
{
	text1 0x0 :
	{
		start.o
	}=0xaabbccdd

	text2 0x30000000 : AT(4096)
/*	text2 0x30000000 :*/
	{
		main.o
	}=0x11223344
}

/*
SECTIONS
{
	.text :
	{
		start.o
		*(.text)
	}
	
	. = ALIGN(4);
	
	.date :
	{
		*(.date)
	}
	
	.bss :
	{
		*(.bss)
	}
}
*/