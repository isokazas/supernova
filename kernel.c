#include "graphics.h"

static void draw_taskbar(void) {
    gfx_fill_rect(0, 0, GFX_WIDTH, 18, COLOR_LIGHT_BLUE);
    gfx_draw_rect(8, 2, 54, 14, COLOR_WHITE, COLOR_LIGHT_BLUE);
    gfx_draw_string(18, 5, "START", COLOR_WHITE, COLOR_LIGHT_BLUE);

    gfx_fill_rect(0, GFX_HEIGHT - 18, GFX_WIDTH, 18, COLOR_LIGHT_BLUE);
    gfx_fill_rect(260, GFX_HEIGHT - 15, 42, 8, COLOR_GREEN);
}

static void draw_app_icon(int x, int y, uint8_t color) {
    gfx_draw_rect(x, y, 30, 30, COLOR_WHITE, color);
    gfx_fill_rect(x + 8, y + 8, 14, 14, COLOR_WHITE);
}

static void draw_window(int x, int y, int w, int h, const char *title, uint8_t accent) {
    gfx_draw_rect(x, y, w, h, COLOR_WHITE, COLOR_DARK_GRAY);
    gfx_fill_rect(x + 1, y + 1, w - 2, 14, accent);
    gfx_draw_string(x + 6, y + 4, title, COLOR_WHITE, accent);
    gfx_fill_rect(x + 4, y + 22, w - 8, h - 26, COLOR_LIGHT_GRAY);
}

void desktop_demo(void) {
    gfx_clear(COLOR_BLUE);
    draw_taskbar();

    draw_window(24, 32, 160, 96, "FILES", COLOR_LIGHT_RED);
    draw_window(204, 52, 92, 92, "INFO", COLOR_LIGHT_GREEN);

    draw_app_icon(40, 60, COLOR_LIGHT_MAGENTA);
    draw_app_icon(40, 108, COLOR_YELLOW);
    draw_app_icon(228, 82, COLOR_LIGHT_CYAN);

    gfx_draw_string(36, 94, "FILES", COLOR_WHITE, COLOR_DARK_GRAY);
    gfx_draw_string(36, 142, "TERM", COLOR_WHITE, COLOR_DARK_GRAY);
    gfx_draw_string(224, 116, "INFO", COLOR_WHITE, COLOR_DARK_GRAY);

    gfx_draw_string(30, 170, "SUPERNOVA DESKTOP", COLOR_WHITE, COLOR_BLUE);
}
