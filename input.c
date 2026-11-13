#include "input.h"
#include "keyboard.h"
#include "ports.h"

#define KBD_STATUS 0x64
#define KBD_DATA 0x60

static unsigned char mouse_packet[3];
static unsigned int mouse_packet_index;
static int extended_scancode;

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
        if (port_byte_in(KBD_STATUS) & 1) {
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
    shift_held = 0;
    if (!wait_input_empty()) return;
    port_byte_out(KBD_STATUS, 0xA8);
    if (!wait_input_empty()) return;
    port_byte_out(KBD_STATUS, 0x20);
    if (!wait_output_full()) return;
    unsigned char command_byte = port_byte_in(KBD_DATA);
    command_byte &= (unsigned char)~0x20;
    command_byte &= (unsigned char)~0x02;
    if (!wait_input_empty()) return;
    port_byte_out(KBD_STATUS, 0x60);
    if (!wait_input_empty()) return;
    port_byte_out(KBD_DATA, command_byte);
    (void)mouse_command(0xF4);
}

int input_poll(input_event_t *event) {
    event->type = INPUT_NONE;
    event->key = KEY_NONE;
    event->character = 0;
    event->mouse_dx = event->mouse_dy = 0;
    event->mouse_buttons = 0;

    unsigned char status = port_byte_in(KBD_STATUS);
    if (!(status & 1)) return 0;
    unsigned char data = port_byte_in(KBD_DATA);
    if (status & 0x20) {
        if (mouse_packet_index == 0 && !(data & 0x08)) return 0;
        mouse_packet[mouse_packet_index++] = data;
        if (mouse_packet_index < 3) return 0;
        mouse_packet_index = 0;
        event->type = INPUT_MOUSE;
        event->mouse_buttons = mouse_packet[0] & 7;
        if (!(mouse_packet[0] & 0x40)) event->mouse_dx = (int)(int8_t)mouse_packet[1];
        if (!(mouse_packet[0] & 0x80)) event->mouse_dy = -(int)(int8_t)mouse_packet[2];
        return 1;
    }
    if (data == 0xE0) { extended_scancode = 1; return 0; }
    if (extended_scancode) {
        extended_scancode = 0;
        if (data & 0x80) return 0;
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
    char c = scancode_to_ascii(data);
    if (!c) return 0;
    event->type = INPUT_KEY;
    if (c == '\n') event->key = KEY_ENTER;
    else if (c == '\b') event->key = KEY_BACKSPACE;
    else if (c == '\t') event->key = KEY_TAB;
    else event->character = c;
    return 1;
}
