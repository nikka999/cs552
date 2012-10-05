.code16

.text
.globl _start

_start:
# Setup stack
	movw $0x9000, %AX
	movw %AX, %SS
	xor %SP, %SP

# Setup DS
	movw $0x7c0, %DX
	movw %DX, %DS

# Load msg pointer into SI, relocate the linker 0x1000 location
	leaw msg-0x1000, %SI
	movw msg_len-0x1000, %CX
	call print_msg

##
# Get system memory
	call get_mem
##

# load unit pointer into SI
	leaw msg_unit-0x1000, %SI
	movw msg_unit_len-0x1000, %CX
	call print_msg

# Inif loop to stop
1:
	jmp 1b

# Get_mem function uses INT 0x15, AX=E801h to get memory size
# www.uruk.org/orig-grub/mem64mb.html
get_mem:
	movw $0xE801, %ax
	int $0x15
	# AX now holds the # of KBs between 1~16 MB
	# BX now holds the # of 64KB blocks above 16 MB

	# Account for >1 MB 
	addw $0x400, %AX
# CANNOT LOAD AX AND BX INTO AL BECAUSE BOTH ARE 2BYTE 
#	movb $0xE, %AH
#	movw %AX, %AL
	#next line is just for testing
	movw $0xa410, %AX
	movw %AX, %BX
	movw $0x2710, %CX
	and $0x0, %DX
	div %CX
	addb $0x30, %AL
	movb 0xE, %AH
	int $0x10
#	movw %BX, %AX
#	movw $0xa, %CX
#	mul %CX
#	movw $0x2710, %CX
#	and $0x0, %DX
#	div %CX
#	add $0x30, %AL
#	movb 0xE, %AH
#	int $0x10
#	jmp L1
#	jmp print_mem
#	and $0x0, %DX
#	movb $0x64, %DL
#	movw $0x2710, %CX
#	div %CX
#	and $0x0, %AH
#	div %DL
#	addb $0x30, %AL
#	movb $0xE, %AH
#	int $0x10
	movb $' ', %AL
	movb $0xE, %AH
	int $0x10
#	movb %BX, %AL
#	int $0x10
	ret
# Print_msg function print whatever SI pointed to with CX length
print_msg:
	lodsb
	or %AL, %AL
	jz return
	movb $0xE, %AH
	int $0x10
	jmp print_msg

#print_mem:
#	movw %AX, %BX
#	movw $0x2710, %CX
#	and $0x0, %DX
#	div %CX
#	add $0x30, %AL
#	movb 0xE, %AH
#	int $0x10
#	movw %BX, %AX
#	movw $0xa, %CX
#	mul %CX
#	movw $0x2710, %CX
#	and $0x0, %DX
#	div %CX
#	add $0x30, %AL
#	movb 0xE, %AH
#	int $0x10
#	jmp L1
return:
	ret

msg: .asciz "MemOS: Welcome *** System Memory is: "
msg_len: .word . - msg
msg_unit: .asciz "KB"
msg_unit_len: .word . - msg_unit

.org 0x1FE, 0x00
.byte 0x55
.byte 0xAA

