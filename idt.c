#include <stdint.h>

#define CODE_SEG 0x08
#define NUM_IDT_ENTRIES 256

typedef struct {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  zero;
    uint8_t  type_attr;
    uint16_t offset_high;
} __attribute__((packed)) idt_entry_t;

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idt_descriptor_t;

static idt_entry_t idt[NUM_IDT_ENTRIES];

/* Forward declare the ASM stub */
extern void irq1_stub(void);

void idt_set_entry(int index, void *handler, uint8_t flags) {
    uint32_t addr = (uint32_t)handler;
    idt[index].offset_low = addr & 0xFFFF;
    idt[index].offset_high = (addr >> 16) & 0xFFFF;
    idt[index].selector = CODE_SEG;
    idt[index].zero = 0;
    idt[index].type_attr = flags;
}

void load_idt(void) {
    idt_descriptor_t desc;
    desc.limit = (sizeof(idt) - 1);
    desc.base = (uint32_t)&idt;

    __asm__ volatile ("lidt %0" : : "m"(desc));
}

void setup_idt(void) {
    /* Set up IDT entries (all default to 0 for now) */
    for (int i = 0; i < NUM_IDT_ENTRIES; i++) {
        idt_set_entry(i, 0, 0);
    }

    /* IRQ1 (keyboard) = interrupt vector 0x21 (32 + 1) */
    idt_set_entry(0x21, irq1_stub, 0x8E);

    load_idt();
}
