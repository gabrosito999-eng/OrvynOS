.section .multiboot
.align 4
.long 0x1BADB002
.long 0x00000000
.long -(0x1BADB002)

.section .text
.global _start
.type _start, @function
_start:
    mov $stack_top, %esp
    call kernel_main
    cli
1:  hlt
    jmp 1b

.section .bss
.align 16
stack_bottom:
.skip 16384
stack_top:

.section .note.GNU-stack,"",@progbits