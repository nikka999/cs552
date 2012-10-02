.text
.globl _start
.code16

_start:
	movb $0xE, %ah
	movb $'H', %al
	int $0x10
	movb $'e', %al
	int $0x10
	movb $'l', %al
	int $0x10
	int $0x10
        movb $'o', %al
        int $0x10
        movb $' ', %al
        int $0x10
        movb $'W', %al
        int $0x10
        movb $'o', %al
        int $0x10
        movb $'r', %al
        int $0x10
        movb $'l', %al
        int $0x10
        movb $'d', %al
        int $0x10
        movb $'!', %al
        int $0x10
	
	ret

.org 0x1FE, 0x90

boot_flag:
.word 0xAA55
