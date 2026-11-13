#include "input.h"
#include "ports.h"

#define KBD_STATUS 0x64
#define KBD_DATA 0x60

static unsigned char mouse_packet[3];
static unsigned int mouse_packet_index;
static int extended_scancode;
static int shift_down;

static int wait_input_empty(void) {
    for (unsigned int i = 0; i < 100000; ++i)
        if (!(port_byte_in(KBD_STATUS) & 0x02)) return 1;
    return 0;
}
static int wait_output_full(void) {
    for (unsigned int i = 0; i < 100000; ++i)
        if (port_byte_in(KBD_STATUS) & 0x01) return 1;
    return 0;
}

static int mouse_command(unsigned char command) {
    if (!wait_input_empty()) return 0;
    port_byte_out(KBD_STATUS, 0xD4);
    if (!wait_input_empty()) return 0;
    port_byte_out(KBD_DATA, command);
    for (unsigned int i = 0; i < 100000; ++i) {
        if (port_byte_in(KBD_STATUS) & 0x01) {
            unsigned char status = port_byte_in(KBD_STATUS);
            unsigned char response = port_byte_in(KBD_DATA);
            if ((status & 0x20) && response == 0xFA) return 1;
        }
    }
    return 0;
}

void input_init(void) {
    mouse_packet_index = 0;
    extended_scancode = 0;
    shift_down = 0;
    if (!wait_input_empty()) return;
    port_byte_out(KBD_STATUS, 0xA8); /* enable auxiliary device */
    if (!wait_input_empty()) return;
    port_byte_out(KBD_STATUS, 0x20); /* read controller command byte */
    if (!wait_output_full()) return;
    unsigned char command_byte = port_byte_in(KBD_DATA);
    command_byte &= (unsigned char)~0x20; /* enable mouse clock */
    command_byte &= (unsigned char)~0x02; /* poll keyboard, no IRQ1 */
    if (!wait_input_empty()) return;
    port_byte_out(KBD_STATUS, 0x60);
    if (!wait_input_empty()) return;
    port_byte_out(KBD_DATA, command_byte);
    (void)mouse_command(0xF4); /* mouse support is optional */
}

static char scancode_char(unsigned char scan, int shift) {
    /* Set-1 AZERTY letters (consistent with the existing keyboard tables). */
    static const char normal[] = "azertyuiopqsdfghjklmwxcvbn";
    static const unsigned char codes[] = {
        0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,
        0x1E,0x1F,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,
        0x2C,0x2D,0x2E,0x2F,0x30,0x31
    };
    for (unsigned int i = 0; i < sizeof(codes); ++i) {
        if (codes[i] == scan) {
            char c = normal[i];
            if (shift && c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
            return c;
        }
    }
    if (scan >= 0x02 && scan <= 0x0B) {
        static const char digits[] = "&e\"'(-_e!c";
        static const char shifted[] = "1234567890";
        return shift ? shifted[scan - 0x02] : digits[scan - 0x02];
    }
    switch (scan) {
        case 0x39: return ' ';
        case 0x0C: return shift ? '+' : ')';
        case 0x1A: return shift ? '^' : '^';
        case 0x1B: return shift ? '$' : '$';
        case 0x27: return shift ? '%' : 'm';
        case 0x28: return shift ? 'u' : 'u';
        case 0x29: return shift ? '2' : ' ';
        case 0x33: return shift ? '.' : ',';
        case 0x34: return shift ? '/' : ';';
        case 0x35: return shift ? '*' : ':';
        default: return 0;
    }
}

int input_poll(input_event_t *event) {
    event->type = INPUT_NONE;
    event->key = KEY_NONE;
    event->character = 0;
    event->mouse_dx = event->mouse_dy = 0;
    event->mouse_buttons = 0;

    unsigned char status = port_byte_in(KBD_STATUS);
    if (!(status & 0x01)) return 0;
    unsigned char data = port_byte_in(KBD_DATA);
    if (status & 0x20) {
        if (mouse_packet_index == 0 && !(data & 0x08)) return 0;
        mouse_packet[mouse_packet_index++] = data;
        if (mouse_packet_index < 3) return 0;
        mouse_packet_index = 0;
        event->type = INPUT_MOUSE;
        event->mouse_buttons = mouse_packet[0] & 0x07;
        if (!(mouse_packet[0] & 0x40)) event->mouse_dx = (int)(int8_t)mouse_packet[1];
        if (!(mouse_packet[0] & 0x80)) event->mouse_dy = -(int)(int8_t)mouse_packet[2];
        return 1;
    }

    if (data == 0xE0) { extended_scancode = 1; return 0; }
    if (data == 0x2A || data == 0x36) { shift_down = 1; return 0; }
    if (data == 0xAA || data == 0xB6) { shift_down = 0; return 0; }
    if (data & 0x80) { extended_scancode = 0; return 0; }
    int extended = extended_scancode;
    extended_scancode = 0;

    if (extended) {
        switch (data) {
            case 0x48: event->key = KEY_UP; break;
            case 0x50: event->key = KEY_DOWN; break;
            case 0x4B: event->key = KEY_LEFT; break;
            case 0x4D: event->key = KEY_RIGHT; break;
            default: return 0;
        }
        event->type = INPUT_KEY;
        return 1;
    }
    switch (data) {
        case 0x0F: event->key = KEY_TAB; break;
        case 0x1C: event->key = KEY_ENTER; break;
        case 0x0E: event->key = KEY_BACKSPACE; break;
        case 0x2D: event->key = KEY_CLOSE; break;
        default: event->character = scancode_char(data, shift_down); break;
    }
    if (event->key == KEY_NONE && !event->character) return 0;
    event->type = INPUT_KEY;
    return 1;
}
