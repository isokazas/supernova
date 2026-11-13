; disk_load.s
[bits 16]
load_kernel:
    mov bx, KERNEL_OFFSET
    mov dh, 100
    mov dl, [BOOT_DRIVE]
    call disk_load
    ret

disk_load:
    push dx
    mov ah, 0x02
    mov al, dh
    mov ch, 0x00
    mov dh, 0x00
    mov cl, 0x02            ; start reading from sector 2
    int 0x13
    jc disk_error
    pop dx
    cmp al, dh
    jne sectors_error
    ret

disk_error:
    mov si, DISK_ERROR_MSG
    call print_string
    jmp $

sectors_error:
    mov si, SECTORS_ERROR_MSG
    call print_string
    jmp $

DISK_ERROR_MSG    db "Disk read error!", 0
SECTORS_ERROR_MSG db "Incorrect sectors read!", 0
