#include "graphics.h"
#include "desktop.h"

void kernel_main(void) {
    gfx_init();
    desktop_demo();

    for (;;) {
        __asm__ volatile ("hlt");
    }
}
