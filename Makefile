all: supernova.img

run: supernova.img
	qemu-system-x86_64 -drive format=raw,file=supernova.img

supernova.img: boot.bin kernel.bin
	cat boot.bin kernel.bin > supernova.img
	dd if=/dev/zero bs=512 count=500 >> supernova.img

boot.bin: boot.s disk_load.s gdt.s 32bit_switch.s print_string.s
	nasm -f bin boot.s -o boot.bin

kernel.bin: entry.o kernel.o terminal_shell.o irq_stubs.o pic.o irq_handlers.o idt.o paging.o heap.o kernel.ld
	ld -m elf_i386 -T kernel.ld --oformat binary entry.o kernel.o terminal_shell.o irq_stubs.o pic.o irq_handlers.o idt.o paging.o heap.o -o kernel.bin

entry.o: entry.s
	nasm -f elf32 entry.s -o entry.o

kernel.o: kernel.c
	gcc -m32 -ffreestanding -fno-stack-protector -fno-pie -fno-builtin -c kernel.c -o kernel.o

terminal_shell.o: terminal_shell.c terminal.c keyboard.h ports.h
	gcc -m32 -ffreestanding -fno-stack-protector -fno-pie -fno-builtin -c terminal_shell.c -o terminal_shell.o

irq_stubs.o: irq_stubs.s
	nasm -f elf32 irq_stubs.s -o irq_stubs.o

pic.o: pic.s
	nasm -f elf32 pic.s -o pic.o

irq_handlers.o: irq_handlers.c ports.h
	gcc -m32 -ffreestanding -fno-stack-protector -fno-pie -fno-builtin -c irq_handlers.c -o irq_handlers.o

idt.o: idt.c
	gcc -m32 -ffreestanding -fno-stack-protector -fno-pie -fno-builtin -c idt.c -o idt.o

paging.o: paging.c
	gcc -m32 -ffreestanding -fno-stack-protector -fno-pie -fno-builtin -c paging.c -o paging.o

heap.o: heap.c
	gcc -m32 -ffreestanding -fno-stack-protector -fno-pie -fno-builtin -c heap.c -o heap.o

clean:
	rm -f *.bin *.o *.img

.PHONY: all run clean
