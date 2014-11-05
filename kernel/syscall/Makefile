CROSS_COMPILE?=arm-none-linux-gnueabi-

CURDIR=$(PWD)
CC = $(CROSS_COMPILE)gcc

#CFLAGS += -Wall -O2 -I./include
#LDFLAGS += -L$(CURDIR)/lib -ljpeg

OBJS = main.o syscall.o
TARGET = syscall 

$(TARGET): $(OBJS)
	$(CC) $^ -o $@ $(LDFLAGS)

clean:
	rm -f $(OBJS) $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.S
	$(CC) $(CFLAGS) -c -o $@ $<
