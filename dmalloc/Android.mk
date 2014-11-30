LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := false

LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := \
	arg_check.c  chunk.c  compat.c  dmallocc.cpp  dmalloc_rand.c \
	dmalloc_tab.c  env.c  error.c  heap.c  malloc.c

LOCAL_CFLAGS += -O2

# enable armv6 idct assembly
LOCAL_CFLAGS += -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 \
				-DHAVE_STDARG_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_UNISTD_H=1 \
				-DHAVE_SYS_MMAN_H=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_W32API_WINBASE_H=0 \
				-DHAVE_W32API_WINDEF_H=0 -DHAVE_SYS_CYGWIN_H=0 -DHAVE_SIGNAL_H=1  \
				-I. -DLOCK_THREADS=1

LOCAL_MODULE:= libdmallocthcxx

#LOCAL_SHARED_LIBRARIES := 

include $(BUILD_SHARED_LIBRARY)
#include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS += -O2 -DDMALLOC -DDMALLOC_FUNC_CHECK -g -include mydmalloc.h
LOCAL_SRC_FILES := dmalloc_test.c
LOCAL_MODULE:= dmalloc_test
#LOCAL_C_INCLUDES := .
LOCAL_SHARED_LIBRARIES := libcutils libdmallocthcxx libc
include $(BUILD_EXECUTABLE)
