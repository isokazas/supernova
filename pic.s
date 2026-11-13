[bits 32]

global init_pic

init_pic:
    ; ICW1
    mov al, 0x11
    out 0x20, al
    out 0xA0, al

    ; ICW2
    mov al, 0x20
    out 0x21, al
    mov al, 0x28
    out 0xA1, al

    ; ICW3
    mov al, 0x04
    out 0x21, al
    mov al, 0x02
    out 0xA1, al

    ; ICW4
    mov al, 0x01
    out 0x21, al
    out 0xA1, al

    ; OCW1 (mask all except IRQ1)
    ; IRQ1 is bit 1, so we want 0xFD (11111101 = all masked except bit 1)
    mov al, 0xFD
    out 0x21, al
    mov al, 0xFF
    out 0xA1, al           ; Mask all slave IRQs

    ret
