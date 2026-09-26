[bits 32]
section .note.GNU-stack noalloc noexec nowrite progbits
section .multiboot
align 4
    dd 0x1BADB002
    dd 0x00
    dd -(0x1BADB002 + 0x00)

section .text
global _start
extern kernel_main

_start:
    cli
    mov esp, stack_top
    call kernel_main
    hlt
    jmp $

section .bss
align 16
stack_bottom:
    resb 16384
stack_top: