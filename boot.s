[org 0x7c00]
[bits 16]

KERNEL_OFFSET equ 0x10000

    ; BIOS does not guarantee these segment registers or the stack.
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00
    sti

    mov [BOOT_DRIVE], dl

    mov si, MSG_BOOTING
    call print_string

    ; Start with a simple VGA graphics mode: 320x200 8-bit indexed color.
    call set_vga_13h

    call load_kernel
    call switch_to_pm
    jmp $

%include "print_string.s"
%include "disk_load.s"
%include "gdt.s"
%include "32bit_switch.s"

set_vga_13h:
    mov ax, 0x0013
    int 0x10
    ret

[bits 32]
BEGIN_PM:
    ; The kernel is linked and loaded at physical address 0x10000.
    mov eax, KERNEL_OFFSET
    call eax
    jmp $

BOOT_DRIVE    db 0
MSG_BOOTING   db "Booting to SupernovaOS...", 0xd, 0xa, 0

 times 510-($-$$) db 0
dw 0xaa55
