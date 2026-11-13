; gdt.s - Global Descriptor Table
gdt_start:

gdt_null: ; Mandatory 8-byte null descriptor
    dq 0x0

gdt_code: ; Code segment descriptor
    dw 0xffff     ; Limit (low)
    dw 0x0        ; Base (low)
    db 0x0        ; Base (middle)
    db 10011010b  ; Access byte (present, ring 0, code, executable, readable)
    db 11001111b  ; Granularity (4KB blocks, 32-bit pmode)
    db 0x0        ; Base (high)

gdt_data: ; Data segment descriptor
    dw 0xffff     ; Limit (low)
    dw 0x0        ; Base (low)
    db 0x0        ; Base (middle)
    db 10010010b  ; Access byte (present, ring 0, data, writable)
    db 11001111b  ; Granularity
    db 0x0        ; Base (high)

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1 ; Size of GDT
    dd gdt_start               ; Start address of GDT

; Constants for segment registers
CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start
