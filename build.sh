#!/bin/bash
set -e
echo "[OrvynOS] Building.bin..."
nasm -f elf32 entry.asm -o entry.o
gcc -m32 -ffreestanding -nostdlib -nostartfiles -fno-builtin -fno-stack-protector -c kernel.c -o kernel.o
gcc -m32 -ffreestanding -nostdlib -nostartfiles -fno-builtin -fno-stack-protector -c fs.c -o fs.o

ld -m elf_i386 -T linker.ld -o kernel.elf entry.o kernel.o fs.o
objcopy -O binary kernel.elf kernel.bin

echo "[OK] kernel.bin listo"
qemu-system-i386 -kernel kernel.elf -netdev user,id=net0 -device e1000,netdev=net0 -serial mon:stdio -m 128M