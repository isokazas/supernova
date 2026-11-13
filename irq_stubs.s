[bits 32]

extern irq1_handler

global irq1_stub

; IRQ1 handler stub (keyboard)
; Called from IDT, saves context, calls C handler, restores context
irq1_stub:
    pusha                   ; Push all general-purpose registers
    cld                     ; Clear direction flag

    call irq1_handler       ; Call C handler (from irq_handlers.c)

    popa                    ; Restore all general-purpose registers
    iret                    ; Return from interrupt
