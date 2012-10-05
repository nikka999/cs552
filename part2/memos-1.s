.text
.globl _start

_start:
	jmp real_start
	.align 4
	.long 0x1BADB002
	.long 0x00000003
	.long 0xE4524FFB

real_start:
	movl $0x07690748, 0xb8000
