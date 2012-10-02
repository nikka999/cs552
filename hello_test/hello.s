.globl _start
.code16

_start:
	movw $msg, %si
	call print_loop

print_loop:
	movb $0xe, %ah
	movb (%si), %al
	cmpb $0, %al
	je .done
	int $0x10
	addw $1, %si
	jmp print_loop

.done:
	hlt
	# NOTHING

msg:
.ascii "MemOS: Welcome *** System Memory is: XXXMB"
.byte 0x0



.org 0x1FE, 0x90
boot_flag: .word 0xAA55
