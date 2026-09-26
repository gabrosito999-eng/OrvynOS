gcc -m32 -c boot.s -o boot.o
gcc -m32 -c kernel.c -o kernel.o -ffreestanding -O2 -nostdlib
gcc -m32 -c fs.c -o fs.o -ffreestanding -O2 -nostdlib
ld -m elf_i386 -T linker.ld -o kernel.elf boot.o kernel.o fs.o
qemu-system-i386 -kernel kernel.elf -vga std