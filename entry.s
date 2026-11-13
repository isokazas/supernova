[bits 32]

extern kernel_main

global _start
_start:
    ; Write 'A' to VGA directly
    mov dword [0xB8000], 0x0F41  ; 0x41 = 'A'

    ; Initialize stack
    mov esp, 0x8000
    mov ebp, esp

    call kernel_main
    jmp $
