#ifndef KEYBOARD_IRQ_H
#define KEYBOARD_IRQ_H

#include "ports.h"

// AZERTY layout with / and * fixes
static char scancode_ascii[] = {
    0,  27, '&', 'e', '"', '\'', '(', '-', 'e', '_', 'c', 'a', ')', '=', '\b',
    '\t', 'a', 'z', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '^', '$', '\n',
    0, 'q', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', 'm', 'u', '*',
    0, '<', 'w', 'x', 'c', 'v', 'b', 'n', ',', ';', '/', '!', 0,
    '*', 0, ' '
};

static char scancode_ascii_shift[] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', 0,   '+', '\b',
    '\t', 'A', 'Z', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', 0,   0,   '\n',
    0, 'Q', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', 'M', '%', 0,
    0, '>', 'W', 'X', 'C', 'V', 'B', 'N', '?', '.', '*', 0, 0,
    '*', 0, ' '
};

#define SC_LSHIFT_DOWN 0x2A
#define SC_RSHIFT_DOWN 0x36
#define SC_LSHIFT_UP   0xAA
#define SC_RSHIFT_UP   0xB6

static int shift_held = 0;

static char scancode_to_ascii(unsigned char scancode) {
    if (scancode == SC_LSHIFT_DOWN || scancode == SC_RSHIFT_DOWN) {
        shift_held = 1;
        return 0;
    }
    if (scancode == SC_LSHIFT_UP || scancode == SC_RSHIFT_UP) {
        shift_held = 0;
        return 0;
    }

    if (scancode & 0x80) return 0;  /* Key release */

        if (scancode < sizeof(scancode_ascii)) {
            return shift_held ? scancode_ascii_shift[scancode] : scancode_ascii[scancode];
        }
        return 0;
}

#endif
