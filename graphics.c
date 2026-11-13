#include "graphics.h"

static volatile uint8_t *framebuffer = (volatile uint8_t *)VGA_GRAPHICS_BASE;

void gfx_init(void) {
    /* BIOS mode 13h already sets a 320x200 8bpp framebuffer at 0xA0000. */
    gfx_clear(0x00);
}

void gfx_clear(uint8_t color) {
    for (int i = 0; i < GFX_WIDTH * GFX_HEIGHT; i++) {
        framebuffer[i] = color;
    }
}

void gfx_put_pixel(int x, int y, uint8_t color) {
    if (x < 0 || y < 0 || x >= GFX_WIDTH || y >= GFX_HEIGHT) {
        return;
    }
    framebuffer[y * GFX_WIDTH + x] = color;
}

void gfx_fill_rect(int x, int y, int w, int h, uint8_t color) {
    if (w <= 0 || h <= 0) {
        return;
    }

    for (int py = y; py < y + h; py++) {
        if (py < 0 || py >= GFX_HEIGHT) {
            continue;
        }
        for (int px = x; px < x + w; px++) {
            if (px < 0 || px >= GFX_WIDTH) {
                continue;
            }
            framebuffer[py * GFX_WIDTH + px] = color;
        }
    }
}

void gfx_draw_rect(int x, int y, int w, int h, uint8_t border_color, uint8_t fill_color) {
    gfx_fill_rect(x, y, w, h, fill_color);

    for (int px = x; px < x + w; px++) {
        gfx_put_pixel(px, y, border_color);
        gfx_put_pixel(px, y + h - 1, border_color);
    }

    for (int py = y; py < y + h; py++) {
        gfx_put_pixel(x, py, border_color);
        gfx_put_pixel(x + w - 1, py, border_color);
    }
}
