#include "desktop.h"
#include "graphics.h"

void kernel_main(void) {
    gfx_init();
    desktop_init();
    desktop_run();
    for (;;) __asm__ volatile ("hlt");
}
