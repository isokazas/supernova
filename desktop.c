#include "graphics.h"

static void draw_bar(int x, int y, int w, int h, uint8_t c) {
    gfx_fill_rect(x, y, w, h, c);
}

static void draw_window(int x, int y, int w, int h, const char *title) {
    (void)title;
    gfx_draw_rect(x, y, w, h, 0x0F, 0x28);
    gfx_fill_rect(x + 1, y + 1, w - 2, 12, 0x2A);
    gfx_fill_rect(x + 4, y + 20, w - 8, h - 24, 0x38);
}

void desktop_demo(void) {
    /* Background */
    gfx_clear(0x10); /* dark blue */

    /* Top bar */
    gfx_fill_rect(0, 0, GFX_WIDTH, 18, 0x1E);

    /* Start panel */
    gfx_draw_rect(8, 2, 52, 14, 0x0F, 0x2D);
    gfx_fill_rect(12, 5, 44, 8, 0x2D);

    /* Big desktop window */
    draw_window(20, 30, 170, 120, "Files");
    draw_window(210, 50, 90, 100, "Info");

    /* App icons */
    gfx_draw_rect(30, 55, 28, 28, 0x0F, 0x29);
    gfx_draw_rect(30, 95, 28, 28, 0x0F, 0x2C);
    gfx_draw_rect(240, 75, 28, 28, 0x0F, 0x2B);

    /* Bottom taskbar */
    gfx_fill_rect(0, GFX_HEIGHT - 18, GFX_WIDTH, 18, 0x1E);

    /* Simple status indicator */
    gfx_fill_rect(275, GFX_HEIGHT - 12, 32, 6, 0x0F);
}
