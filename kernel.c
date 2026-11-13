#include <stdint.h>

#define VGA ((volatile uint16_t*)0xB8000)

void kernel_main() {
    VGA[0] = (uint16_t)('A' | (0x0F << 8));
    VGA[1] = (uint16_t)('B' | (0x0F << 8));
    VGA[2] = (uint16_t)('C' | (0x0F << 8));

    while (1) __asm__ volatile ("hlt");
}
