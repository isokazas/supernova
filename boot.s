[org 0x7c00]
[bits 16]

KERNEL_OFFSET equ 0x1000

    mov [BOOT_DRIVE], dl
    mov bp, 0x9000
    mov sp, bp

    mov si, MSG_BOOTING
    call print_string

    call load_kernel
    call switch_to_pm
    jmp $

%include "print_string.s"
%include "disk_load.s"
%include "gdt.s"
%include "32bit_switch.s"

[bits 32]
BEGIN_PM:
    call KERNEL_OFFSET
    jmp $

BOOT_DRIVE    db 0
MSG_BOOTING   db "Booting to SupernovaOS...", 0xd, 0xa, 0

times 510-($-$$) db 0
dw 0xaa55
