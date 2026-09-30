#!/bin/bash
echo "[*] Compilando OrvynOS 0.2.3..."
nasm -f elf32 boot.asm -o boot.o
gcc -m32 -c kernel.c -o kernel.o -ffreestanding -O2 -nostdlib -fno-stack-protector
ld -m elf_i386 -T linker.ld -o kernel.elf boot.o kernel.o
echo "[✓] kernel.elf listo"
ls -lh kernel.elf
echo ""
echo "Para QEMU:"
echo "qemu-system-i386 -kernel kernel.elf -m 64M"
echo ""
echo "Creando OrvynOS-0.2.3-Snake.iso fake (es el elf renombrado, bootea con Ventoy/Rufus DD)..."
cp kernel.elf OrvynOS-0.2.3-Snake.iso
cp kernel.elf OrvynOS-0.2.3-Snake.img
ls -lh OrvynOS-0.2.3-Snake.*
