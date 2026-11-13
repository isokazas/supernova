#include <stdint.h>
#include "graphics.h"
#include "terminal_gui.h"

/* Reuse the existing shell, fake filesystem, command parser, and VGA text console. */
#include "terminal.c"

#define GUI_COLS 80
#define GUI_ROWS 25
#define GUI_TEXT_MEMORY ((volatile uint16_t *)0xB8000)

static char input_line[LINE_MAX];
static size_t input_length;

void terminal_gui_init(void) {
    input_length = 0;
    input_line[0] = '\0';
    terminal_init();
    terminal_print_prompt();
}

void terminal_gui_input(char c) {
    if (c == '\n') {
        term_putc('\n');
        input_line[input_length] = '\0';
        terminal_process_line(input_line);
        input_length = 0;
        input_line[0] = '\0';
        terminal_print_prompt();
    } else if (c == '\b') {
        if (input_length) {
            --input_length;
            input_line[input_length] = '\0';
            term_putc('\b');
        }
    } else if (c >= 32 && c <= 126 && input_length < LINE_MAX - 1) {
        input_line[input_length++] = c;
        input_line[input_length] = '\0';
        term_putc(c);
    }
}

static uint8_t vga_color(unsigned int index) {
    static const uint8_t colors[16] = {
        COLOR_BLACK, COLOR_BLUE, COLOR_GREEN, COLOR_CYAN,
        COLOR_RED, COLOR_MAGENTA, COLOR_BROWN, COLOR_LIGHT_GRAY,
        COLOR_DARK_GRAY, COLOR_LIGHT_BLUE, COLOR_LIGHT_GREEN, COLOR_LIGHT_CYAN,
        COLOR_LIGHT_RED, COLOR_LIGHT_MAGENTA, COLOR_YELLOW, COLOR_WHITE
    };
    return colors[index & 15];
}

void terminal_gui_draw(int x, int y, int columns, int rows) {
    if (columns > GUI_COLS) columns = GUI_COLS;
    if (rows > GUI_ROWS) rows = GUI_ROWS;
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < columns; ++col) {
            uint16_t cell = GUI_TEXT_MEMORY[row * GUI_COLS + col];
            char ch = (char)(cell & 0xFF);
            uint8_t attr = (uint8_t)(cell >> 8);
            if (ch < 32 || ch > 122) ch = ' ';
            gfx_draw_char(x + col * 6, y + row * 8, ch,
                          vga_color(attr & 15), vga_color(attr >> 4));
        }
    }
}
