#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdint.h>

#define GFX_WIDTH 320
#define GFX_HEIGHT 200
#define VGA_GRAPHICS_BASE 0xA0000

#define COLOR_BLACK 0x00
#define COLOR_BLUE 0x01
#define COLOR_GREEN 0x02
#define COLOR_CYAN 0x03
#define COLOR_RED 0x04
#define COLOR_MAGENTA 0x05
#define COLOR_BROWN 0x06
#define COLOR_LIGHT_GRAY 0x07
#define COLOR_DARK_GRAY 0x08
#define COLOR_LIGHT_BLUE 0x09
#define COLOR_LIGHT_GREEN 0x0A
#define COLOR_LIGHT_CYAN 0x0B
#define COLOR_LIGHT_RED 0x0C
#define COLOR_LIGHT_MAGENTA 0x0D
#define COLOR_YELLOW 0x0E
#define COLOR_WHITE 0x0F

void gfx_init(void);
void gfx_clear(uint8_t color);
void gfx_put_pixel(int x, int y, uint8_t color);
void gfx_fill_rect(int x, int y, int w, int h, uint8_t color);
void gfx_draw_rect(int x, int y, int w, int h, uint8_t border_color, uint8_t fill_color);
void gfx_draw_char(int x, int y, char c, uint8_t fg, uint8_t bg);
void gfx_draw_string(int x, int y, const char *s, uint8_t fg, uint8_t bg);

#endif
