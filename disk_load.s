; Load the first 100 sectors after the boot sector with BIOS EDD (LBA).
; The destination is physical 0x10000, away from the boot stack at 0x7c00.
[bits 16]
load_kernel:
    mov si, DISK_ADDRESS_PACKET
    mov dl, [BOOT_DRIVE]
    mov ah, 0x42
    int 0x13
    jc disk_error
    ret

disk_error:
    mov si, DISK_ERROR_MSG
    call print_string
.hang:
    jmp .hang

align 4
DISK_ADDRESS_PACKET:
    db 0x10, 0                 ; packet size, reserved
    dw 100                     ; sectors to read (maximum supported here)
    dw 0x0000                  ; destination offset
    dw 0x1000                  ; destination segment = physical 0x10000
    dq 1                       ; start at LBA 1 (sector after boot sector)

DISK_ERROR_MSG db "Disk read error!", 0xd, 0xa, 0
