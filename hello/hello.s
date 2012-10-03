.code16

.text
.globl _start

msg: .asciz "HELLO"
msg_len: .word . - msg

_start:
	movw $0x9000, %ax
	movw %ax, %ss
	xor %sp, %sp
	movw $0x7c0, %dx
	movw %dx, %ds
	leaw msg-0x1000, %si
	movw msg_len-0x1000, %cx

loop:
	lodsb
	or %al, %al
	jz fin
	movb $0xE, %ah
	int $0x10
	jmp loop

fin:
#do nothing
	jmp fin

.org 0x1FE, 0x00
.byte 0x55
.byte 0xAA

