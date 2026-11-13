#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdint.h>

#define GFX_WIDTH 320
#define GFX_HEIGHT 200
#define VGA_GRAPHICS_BASE 0xA0000

void gfx_init(void);
void gfx_clear(uint8_t color);
void gfx_put_pixel(int x, int y, uint8_t color);
void gfx_fill_rect(int x, int y, int w, int h, uint8_t color);
void gfx_draw_rect(int x, int y, int w, int h, uint8_t border_color, uint8_t fill_color);

#endif
