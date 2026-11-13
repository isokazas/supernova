#include <stdint.h>

extern void terminal_init(void);
extern void terminal_run(void);

void kernel_main(void) {
    terminal_init();
    terminal_run();

    for (;;) {
        __asm__ volatile ("hlt");
    }
}
