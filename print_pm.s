; print_pm.s
[bits 32]
VIDEO_MEMORY equ 0xb8000
WHITE_ON_BLACK equ 0x0f

print_string_pm:
    pusha
    mov edx, VIDEO_MEMORY

.loop:
    mov al, [ebx]           ; Load character from EBX pointer
    mov ah, WHITE_ON_BLACK  ; Load style attributes

    cmp al, 0               ; Check for null terminator
    je .done

    mov [edx], ax           ; Write character + attribute to VGA memory
    add ebx, 1              ; Next character in string
    add edx, 2              ; Next character cell on screen (2 bytes each)
    jmp .loop

.done:
    popa
    ret
