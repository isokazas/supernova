#include "graphics.h"
#include "ports.h"

static uint8_t backbuffer[GFX_FRAME_BYTES];
static volatile uint8_t *const vga = (volatile uint8_t *)VGA_GRAPHICS_BASE;

/* Seven-row, five-pixel glyphs for A-Z and 0-9. Lowercase is uppercased. */
static const uint8_t glyphs[36][7] = {
    {14,17,17,31,17,17,17}, {30,17,17,30,17,17,30}, {14,17,16,16,16,17,14},
    {30,17,17,17,17,17,30}, {31,16,16,30,16,16,31}, {31,16,16,30,16,16,16},
    {14,17,16,23,17,17,15}, {17,17,17,31,17,17,17}, {14,4,4,4,4,4,14},
    {7,2,2,2,18,18,12}, {17,18,20,24,20,18,17}, {16,16,16,16,16,16,31},
    {17,27,21,21,17,17,17}, {17,25,21,19,17,17,17}, {14,17,17,17,17,17,14},
    {30,17,17,30,16,16,16}, {14,17,17,17,21,18,13}, {30,17,17,30,20,18,17},
    {15,16,16,14,1,1,30}, {31,4,4,4,4,4,4}, {17,17,17,17,17,17,14},
    {17,17,17,17,17,10,4}, {17,17,17,21,21,21,10}, {17,17,10,4,10,17,17},
    {17,17,10,4,4,4,4}, {31,1,2,4,8,16,31},
    {14,17,19,21,25,17,14}, {4,12,4,4,4,4,14}, {14,17,1,2,4,8,31},
    {30,1,1,14,1,1,30}, {2,6,10,18,31,2,2}, {31,16,16,30,1,1,30},
    {14,16,16,30,17,17,14}, {31,1,2,4,8,8,8}, {14,17,17,14,17,17,14},
    {14,17,17,15,1,1,14}
};

void gfx_init(void) { gfx_clear(COLOR_BLACK); gfx_present(); }

void gfx_present(void) {
    /* Wait briefly for vertical retrace if available, but never hang if absent. */
    unsigned int timeout = 100000;
    while (timeout-- && (port_byte_in(0x3DA) & 0x08)) { }
    timeout = 100000;
    while (timeout-- && !(port_byte_in(0x3DA) & 0x08)) { }
    for (int i = 0; i < GFX_FRAME_BYTES; ++i) vga[i] = backbuffer[i];
}

void gfx_clear(uint8_t color) {
    for (int i = 0; i < GFX_FRAME_BYTES; ++i) backbuffer[i] = color;
}

void gfx_put_pixel(int x, int y, uint8_t color) {
    if (x >= 0 && x < GFX_WIDTH && y >= 0 && y < GFX_HEIGHT)
        backbuffer[y * GFX_WIDTH + x] = color;
}

void gfx_fill_rect(int x, int y, int w, int h, uint8_t color) {
    if (w <= 0 || h <= 0) return;
    int x0 = x < 0 ? 0 : x, y0 = y < 0 ? 0 : y;
    int y1 = y + h > GFX_HEIGHT ? GFX_HEIGHT : y + h;
    int x1 = x + w > GFX_WIDTH ? GFX_WIDTH : x + w;
    for (int py = y0; py < y1; ++py)
        for (int px = x0; px < x1; ++px) backbuffer[py * GFX_WIDTH + px] = color;
}

void gfx_draw_rect(int x, int y, int w, int h, uint8_t border, uint8_t fill) {
    if (w <= 0 || h <= 0) return;
    gfx_fill_rect(x, y, w, h, fill);
    for (int px = x; px < x + w; ++px) {
        gfx_put_pixel(px, y, border);
        gfx_put_pixel(px, y + h - 1, border);
    }
    for (int py = y; py < y + h; ++py) {
        gfx_put_pixel(x, py, border);
        gfx_put_pixel(x + w - 1, py, border);
    }
}

static int glyph_index(char c) {
    if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= '0' && c <= '9') return 26 + c - '0';
    return -1;
}

static uint8_t punctuation(char c, int row) {
    switch (c) {
        case ' ': return 0;
        case '.': return row == 6 ? 4 : 0;
        case ',': return row == 6 ? 4 : (row == 5 ? 2 : 0);
        case ':': return (row == 2 || row == 5) ? 4 : 0;
        case ';': return row == 2 ? 4 : (row == 5 ? 4 : (row == 6 ? 2 : 0));
        case '-': return row == 3 ? 14 : 0;
        case '_': return row == 6 ? 31 : 0;
        case '/': return (uint8_t)(1u << (row < 5 ? row : 4));
        case '\\': return (uint8_t)(1u << (4 - (row < 5 ? row : 4)));
        case '>': return row == 2 ? 8 : (row == 3 ? 16 : (row == 4 ? 8 : 0));
        case '<': return row == 2 ? 2 : (row == 3 ? 1 : (row == 4 ? 2 : 0));
        case '=': return (row == 2 || row == 4) ? 14 : 0;
        case '+': return row == 3 ? 14 : (row == 1 || row == 2 || row == 4 || row == 5 ? 4 : 0);
        case '*': return row == 2 ? 10 : (row == 3 ? 4 : (row == 4 ? 10 : 0));
        case '!': return row < 5 ? 4 : (row == 6 ? 4 : 0);
        case '?': return row == 0 ? 14 : (row == 1 ? 17 : (row == 2 ? 2 : (row == 3 ? 4 : (row == 6 ? 4 : 0))));
        case '>': return row == 3 ? 16 : (row == 2 || row == 4 ? 8 : 0);
        case '|': return 4;
        case '#': return row == 2 || row == 4 ? 31 : 10;
        case '(': return row == 0 || row == 6 ? 2 : 4;
        case ')': return row == 0 || row == 6 ? 8 : 4;
        case '[': return row == 0 || row == 6 ? 14 : 8;
        case ']': return row == 0 || row == 6 ? 14 : 2;
        case '\'': return row == 0 || row == 1 ? 4 : 0;
        case '"': return row < 2 ? 10 : 0;
        default: return 0;
    }
}

void gfx_draw_char(int x, int y, char c, uint8_t fg, uint8_t bg) {
    int index = glyph_index(c);
    for (int row = 0; row < 7; ++row) {
        uint8_t bits = index >= 0 ? glyphs[index][row] : punctuation(c, row);
        for (int col = 0; col < 5; ++col)
            gfx_put_pixel(x + col, y + row, (bits & (1u << (4 - col))) ? fg : bg);
    }
}

void gfx_draw_string(int x, int y, const char *text, uint8_t fg, uint8_t bg) {
    int origin_x = x;
    while (*text) {
        if (*text == '\n') { y += 9; x = origin_x; }
        else { gfx_draw_char(x, y, *text, fg, bg); x += 6; }
        ++text;
    }
}
