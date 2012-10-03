.globl _start
.code16

_start:
    leaw msg, $si
    movw msg_len, %cs

1:
    lodsb
    movb $0x0e, %ah
    int $0x10
    loop 1b

2:
    loop2b
    
msg:
    .asciz "MemOS: Welcome *** System Memory is: XXXMB"
msg_len:
    .word . - msg

.org 0x1FE, 0x90
boot_flag: .word 0xAA55
