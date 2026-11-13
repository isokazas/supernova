#include <stdint.h>
#include <stddef.h>

#define VGA_MEMORY ((volatile uint16_t*)0xB8000)

static volatile uint16_t *vga = VGA_MEMORY;
static int col = 0, row = 0;

static void vga_clear(void) {
    for (int i = 0; i < 80 * 25; i++) {
        vga[i] = (uint16_t)(' ' | (0x0F << 8));
    }
    col = 0;
    row = 0;
}

static void vga_putc(char c) {
    if (c == '\n') {
        col = 0;
        row++;
        return;
    }
    vga[row * 80 + col] = (uint16_t)((unsigned char)c | (0x0F << 8));
    col++;
    if (col >= 80) {
        col = 0;
        row++;
    }
}

static void vga_print(const char *s) {
    while (*s) vga_putc(*s++);
}

static void vga_hex(unsigned char val) {
    const char *hex = "0123456789ABCDEF";
    vga_putc(hex[(val >> 4) & 0xF]);
    vga_putc(hex[val & 0xF]);
}

extern void setup_idt(void);
extern void init_pic(void);
extern int kb_getc_nonblock(void);

void kernel_main() {
    for (volatile int i = 0; i < 100000; i++);

    vga_clear();
    vga_print("Scancode Tester\n");
    vga_print("Press keys, note codes\n");
    vga_print("ESC (0x01) to exit\n\n");

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
        vga_print("Code: 0x");
        vga_hex(sc);
        vga_putc('\n');

        if (sc == 0x01) break;
    }
}
