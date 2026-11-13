#include "input.h"
#include "ports.h"

#define KBD_STATUS 0x64
#define KBD_DATA   0x60

static unsigned char mouse_packet[3];
static unsigned int mouse_packet_index;
static int extended_scancode;

static int wait_input_empty(void) {
    for (unsigned int i = 0; i < 100000; ++i)
        if ((port_byte_in(KBD_STATUS) & 0x02) == 0) return 1;
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
            if (!(status & 0x20)) continue; /* discard unrelated keyboard byte */
        }
    }
    return 0;
}

void input_init(void) {
    mouse_packet_index = 0;
    extended_scancode = 0;

    /* Enable the PS/2 auxiliary device and its clock for polled mouse input. */
    if (!wait_input_empty()) return;
    port_byte_out(KBD_STATUS, 0xA8);
    if (!wait_input_empty()) return;
    port_byte_out(KBD_STATUS, 0x20);
    if (!wait_output_full()) return;
    unsigned char command_byte = port_byte_in(KBD_DATA);
    command_byte &= (unsigned char)~0x20; /* clear auxiliary-clock-disable */
    command_byte &= (unsigned char)~0x02; /* polling; do not enable IRQ1 */
    if (!wait_input_empty()) return;
    port_byte_out(KBD_STATUS, 0x60);
    if (!wait_input_empty()) return;
    port_byte_out(KBD_DATA, command_byte);
    (void)mouse_command(0xF4); /* enable mouse data reporting; keyboard still works without it */
}

static key_code_t translate_scancode(unsigned char scan, int extended) {
    if (scan & 0x80) return KEY_NONE; /* key release */
    if (extended) {
        switch (scan) {
            case 0x48: return KEY_UP;
            case 0x50: return KEY_DOWN;
            case 0x4B: return KEY_LEFT;
            case 0x4D: return KEY_RIGHT;
            default: return KEY_NONE;
        }
    }
    switch (scan) {
        case 0x0F: return KEY_TAB;
        case 0x1C: return KEY_ENTER;
        case 0x2D: return KEY_CLOSE; /* X key */
        default: return KEY_NONE;
    }
}

int input_poll(input_event_t *event) {
    event->type = INPUT_NONE;
    event->key = KEY_NONE;
    event->mouse_dx = event->mouse_dy = 0;
    event->mouse_buttons = 0;

    unsigned char status = port_byte_in(KBD_STATUS);
    if (!(status & 0x01)) return 0;

    unsigned char data = port_byte_in(KBD_DATA);
    if (status & 0x20) {
        if (mouse_packet_index == 0 && !(data & 0x08)) return 0; /* packet sync */
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
    int was_extended = extended_scancode;
    extended_scancode = 0;
    key_code_t key = translate_scancode(data, was_extended);
    if (key == KEY_NONE) return 0;
    event->type = INPUT_KEY;
    event->key = key;
    return 1;
}
