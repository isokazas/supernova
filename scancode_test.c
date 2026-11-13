#include <stdint.h>
#include <stddef.h>

#define VGA_MEMORY ((volatile uint16_t*)0xB8000)

static volatile uint16_t *vga = (volatile uint16_t *)0xB8000;
static int debug_col = 0, debug_row = 0;

static void debug_putc(char c) {
    vga[debug_row * 80 + debug_col] = (uint16_t)((unsigned char)c | (0x0F << 8));
    debug_col++;
    if (debug_col >= 80) {
        debug_col = 0;
        debug_row++;
    }
}

static void debug_print(const char *s) {
    while (*s) debug_putc(*s++);
}

static void debug_hex(unsigned char val) {
    char hex[] = "0123456789ABCDEF";
    debug_putc(hex[(val >> 4) & 0xF]);
    debug_putc(hex[val & 0xF]);
}

static void debug_clear(void) {
    volatile uint16_t *vga_mem = VGA_MEMORY;
    for (size_t i = 0; i < 80 * 25; i++) {
        vga_mem[i] = (uint16_t)(' ' | (0x0F << 8));
    }
    debug_col = 0;
    debug_row = 0;
}

extern void setup_idt(void);
extern void init_pic(void);
extern int kb_getc_nonblock(void);

#include "ports.h"

void kernel_main() {
    for (volatile int i = 0; i < 100000; i++);

    debug_clear();

    debug_print("Scancode tester - press keys\n");
    debug_print("Press Escape (key 1) to exit\n\n");

    init_pic();
    setup_idt();
    __asm__ volatile ("sti");

    while (1) {
        int scancode = kb_getc_nonblock();
        if (scancode == -1) {
            __asm__ volatile ("hlt");
            continue;
        }

        unsigned char sc = (unsigned char)scancode;

        debug_print("Scancode: 0x");
        debug_hex(sc);
        debug_print("\n");

        if (sc == 0x01) {
            debug_print("(ESC - exiting)\n");
            break;
        }
    }

    debug_print("Done\n");
    for (;;) __asm__ volatile ("hlt");
}
