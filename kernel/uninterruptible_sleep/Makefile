#KERNEL_PATH = /home/tujian/work/elite-project
KERNEL_PATH = /home/tujian/zx/android_kernel_3.10
#KERNEL_PATH = /home/tujian/zx/elite-project
CUR_DIR = $(shell pwd)

ARCH = arm
CROSS_COMPILE = arm-none-linux-gnueabi-

export ARCH CROSS_COMPILE

CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld

#only one file
obj-m := uninterruptible_sleep.o

#more than one file
#obj-m := module_name.o
#module_name-objs := file1.o file2.o

EXTRA_CFLAGS = -g
#EXTRA_CFLAGS = -mno-unaligned-access -mcpu=cortex-a9

.PHONY: module clean

module:
	make -C $(KERNEL_PATH) M=$(CUR_DIR) modules

clean:
	make -C $(KERNEL_PATH) M=$(CUR_DIR) clean
