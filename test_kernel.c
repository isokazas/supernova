#include <stdint.h>

#define VGA ((volatile uint16_t*)0xB8000)

static int col = 0, row = 0;

static void putc(char c) {
    if (c == '\n') {
        col = 0;
        row++;
        if (row >= 25) row = 0;
        return;
    }
    VGA[row * 80 + col] = (uint16_t)((unsigned char)c | (0x0F << 8));
    col++;
    if (col >= 80) {
        col = 0;
        row++;
        if (row >= 25) row = 0;
    }
}

static void print(const char *s) {
    while (*s) putc(*s++);
}

static void hex(unsigned char v) {
    const char *h = "0123456789ABCDEF";
    putc(h[(v >> 4) & 0xF]);
    putc(h[v & 0xF]);
}

extern void setup_idt(void);
extern void init_pic(void);
extern int kb_getc_nonblock(void);

void kernel_main() {
    /* Clear screen */
    for (int i = 0; i < 80 * 25; i++) VGA[i] = 0x0F20;
    col = 0; row = 0;

    print("SCANCODE TESTER\n");
    print("Press keys:\n\n");

    init_pic();
    setup_idt();
    __asm__ volatile ("sti");

    while (1) {
        int sc = kb_getc_nonblock();
        if (sc == -1) {
            __asm__ volatile ("hlt");
            continue;
        }

        print("0x");
        hex((unsigned char)sc);
        putc('\n');

        if (sc == 0x01) break;
    }

    print("DONE\n");
    while (1) __asm__ volatile ("hlt");
}
