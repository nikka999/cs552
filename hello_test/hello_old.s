	.globl _start
# Declare 16-bit
	.code16

# Entry point
_start:
# Set Stack Segment and clear Stack Pointer 
	movw $0x9000, %ax
	movw %ax, %ss
	xorw %sp, %sp

# Move msg to SI for passing arg then print
	movw $msg, %si
	call printMsg

# Print Message function
printMsg:
	movb $0xE, %ah
	movb (%si), %al
	cmpb $0, %al
	je done
	int $0x10
	addw $1, %si
	jmp printMsg

done:
	# Do nothing

# Messages
msg:
	.ascii "MemOS: Welcome *** System Memory is : XXX MB"
	.byte 0
	

# Fill NOP instruction (opcode = 0x90) from here to the 3rd last byte
	.org 0x1FE, 0x90

# Boot disk signature
	.byte 0x55
	.byte 0xAA


