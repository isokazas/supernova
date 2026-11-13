[bits 32]

extern kernel_main
extern __bss_start
extern __bss_end

global _start
_start:
    ; Start with a known flat protected-mode data segment and stack.
    mov esp, 0x9000
    mov ebp, esp
    cld

    ; Raw kernel images do not contain initialized BSS bytes. Clear them here
    ; so static variables (including the terminal's state) start as zero.
    mov edi, __bss_start
    mov ecx, __bss_end
    sub ecx, edi
    xor eax, eax
    rep stosb

    call kernel_main
.hang:
    hlt
    jmp .hang
