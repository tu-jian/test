#define ALIGN .align 0
#define ALIGN_STR ".align 0"

#define END(name) \
  .size name, .-name

#define ENDPROC(name) \
  .type name, %function; \
  END(name)

#define ENTRY(name) \
  .globl name; \
  ALIGN; \
  name:

#define __NR_getpid 20

ENTRY(my_getpid)
    mov     ip, r7
    ldr     r7, =__NR_getpid
    swi     #0
    mov     r7, ip
    bx      lr
/*
    cmn     r0, #(MAX_ERRNO + 1)
    bxls    lr
    neg     r0, r0
    b       __set_errno
*/
ENDPROC(my_getpid)

