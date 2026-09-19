[bits 32]
global _start
extern kernel_main
_start:
    mov esp, 0x90000
    and esp, 0xFFFFFFF0
    call kernel_main
    cli
hang:
    hlt
    jmp hang