.globl _start
_start:
	mov r0, #0
	mov sp, #4096
#	b main
	ldr r0, =main
	mov pc, r0
	b .
