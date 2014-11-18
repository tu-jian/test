CROESS_COMPILE = arm-none-linux-gnueabi-

CC = $(CROESS_COMPILE)gcc
LD = $(CROESS_COMPILE)ld
OBJCOPY = $(CROESS_COMPILE)objcopy

CFLAGS = -funwind-tables 
SRC_FILES = start.S main.c
DEST_OBJ=start.o main.o

main.bin:$(SRC_FILES)
	$(CC) -c -o start.o start.S $(CFLAGS)
	$(CC) -c -o main.o main.c $(CFLAGS)
	$(LD) -Ttext 0x10000000 $(DEST_OBJ) -Map test.map -o main.elf 
#	$(LD) -Ttest.lds $(DEST_OBJ) -Map test.map -o main.elf
	$(OBJCOPY) -O binary -S main.elf main.bin

clean:
	rm -f *.o *.elf *.bin *.map
