#!/bin/bash
set -e
nasm -f bin boot.asm -o boot.bin
nasm -f elf32 entry.asm -o entry.o
gcc -m32 -ffreestanding -fno-pie -fno-stack-protector -mno-sse -mno-sse2 -O0 -c kernel.c -o kernel.o
ld -m elf_i386 -T link.ld -o kernel.elf entry.o kernel.o
objcopy -O binary kernel.elf kernel.bin
cat boot.bin kernel.bin > orvyn.bin
truncate -s 1474560 orvyn.bin
echo "kernel.bin=$(stat -c%s kernel.bin)"
qemu-system-i386 -drive format=raw,file=orvyn.bin,if=floppy -boot a -display sdl