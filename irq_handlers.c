#include <stdint.h>
#include "ports.h"

/* Keyboard buffer (simple ring buffer) */
#define KB_BUFFER_SIZE 256
static char kb_buffer[KB_BUFFER_SIZE];
static int kb_head = 0, kb_tail = 0;
static int kb_count = 0;

/* IRQ1 handler (keyboard) — called from assembly stub */
void irq1_handler(void) {
    unsigned char scancode = port_byte_in(0x60);

    /* Add to buffer if not full */
    if (kb_count < KB_BUFFER_SIZE) {
        kb_buffer[kb_tail] = scancode;
        kb_tail = (kb_tail + 1) % KB_BUFFER_SIZE;
        kb_count++;
    }

    /* Send EOI to PIC */
    port_byte_out(0x20, 0x20);  /* EOI to master */
}

/* Get next scancode from buffer (non-blocking) */
int kb_getc_nonblock(void) {
    if (kb_count == 0) return -1;

    char c = kb_buffer[kb_head];
    kb_head = (kb_head + 1) % KB_BUFFER_SIZE;
    kb_count--;
    return (unsigned char)c;
}

/* Get next scancode from buffer (blocking) */
unsigned char kb_getc_block(void) {
    while (kb_count == 0) {
        __asm__ volatile ("hlt");  /* Wait for interrupt */
    }
    return kb_getc_nonblock();
}
