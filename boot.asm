[org 0x7c00]
[bits 16]
mov [BOOT_DRIVE], dl
xor ax, ax
mov ds, ax
mov es, ax
mov ss, ax
mov sp, 0x7c00
mov si, MSG_REAL
call print16
xor ax, ax
mov es, ax
mov bx, 0x1000
mov ah, 0x02
mov al, 18
mov ch, 0
mov cl, 2
mov dh, 0
mov dl, [BOOT_DRIVE]
int 0x13
jc disk_error
xor ax, ax
mov es, ax
mov bx, 0x1000
add bx, 0x2400
mov ah, 0x02
mov al, 14
mov ch, 0
mov cl, 1
mov dh, 1
mov dl, [BOOT_DRIVE]
int 0x13
jc disk_error
cli
lgdt [gdt_descriptor]
mov eax, cr0
or eax, 1
mov cr0, eax
jmp CODE_SEG:init_pm
disk_error:
mov si, MSG_DISK_ERR
call print16
hlt
print16:
mov ah, 0x0E
mov bh, 0
.print_loop:
lodsb
test al, al
jz .done
int 0x10
jmp .print_loop
.done:
ret
[bits 32]
init_pm:
mov ax, DATA_SEG
mov ds, ax
mov es, ax
mov fs, ax
mov gs, ax
mov ss, ax
mov esp, 0x90000
jmp 0x1000
gdt_start:
dq 0
gdt_code:
dw 0xFFFF
dw 0
db 0
db 10011010b
db 11001111b
db 0
gdt_data:
dw 0xFFFF
dw 0
db 0
db 10010010b
db 11001111b
db 0
gdt_end:
gdt_descriptor:
dw gdt_end - gdt_start - 1
dd gdt_start
CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start
BOOT_DRIVE db 0
MSG_REAL db 'OrvynOS loading...',13,10,0
MSG_DISK_ERR db 'Disk Error!',0
times 510-($-$$) db 0
dw 0xAA55