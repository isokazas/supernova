#include "desktop.h"
#include "graphics.h"
#include "input.h"

#define WINDOW_COUNT 2
#define TASKBAR_HEIGHT 16
#define CURSOR_STEP 4

typedef struct {
    int x, y, w, h;
    const char *title;
    uint8_t title_color;
    int visible;
} window_t;

static window_t windows[WINDOW_COUNT] = {
    {28, 36, 166, 94, "FILES", COLOR_LIGHT_RED, 1},
    {176, 62, 116, 82, "SYSTEM INFO", COLOR_LIGHT_GREEN, 1}
};
static int focused_window;
static int cursor_x = GFX_WIDTH / 2;
static int cursor_y = GFX_HEIGHT / 2;
static int dragging;
static int drag_dx, drag_dy;
static uint8_t previous_buttons;

static int clamp(int value, int low, int high) {
    return value < low ? low : (value > high ? high : value);
}

static void draw_window(int index) {
    window_t *w = &windows[index];
    if (!w->visible) return;
    uint8_t border = index == focused_window ? COLOR_YELLOW : COLOR_WHITE;
    gfx_draw_rect(w->x, w->y, w->w, w->h, border, COLOR_DARK_GRAY);
    gfx_fill_rect(w->x + 1, w->y + 1, w->w - 2, 13, w->title_color);
    gfx_draw_string(w->x + 5, w->y + 4, w->title, COLOR_WHITE, w->title_color);
    gfx_fill_rect(w->x + 4, w->y + 19, w->w - 8, w->h - 23, COLOR_LIGHT_GRAY);
    if (index == 0) {
        gfx_draw_string(w->x + 10, w->y + 27, "YOUR FILES", COLOR_BLACK, COLOR_LIGHT_GRAY);
        gfx_draw_string(w->x + 10, w->y + 40, "DESKTOP", COLOR_BLUE, COLOR_LIGHT_GRAY);
        gfx_draw_string(w->x + 10, w->y + 52, "TERMINAL", COLOR_BLUE, COLOR_LIGHT_GRAY);
    } else {
        gfx_draw_string(w->x + 10, w->y + 28, "SUPERNOVA", COLOR_BLACK, COLOR_LIGHT_GRAY);
        gfx_draw_string(w->x + 10, w->y + 41, "GRAPHICS DEMO", COLOR_BLACK, COLOR_LIGHT_GRAY);
    }
}

static void draw_cursor(void) {
    /* Small outlined pointer; redraw the scene after every input event. */
    for (int row = 0; row < 10; ++row) {
        for (int col = 0; col <= row / 2; ++col) {
            int x = cursor_x + col, y = cursor_y + row;
            if (row == 9 || col == 0 || col == row / 2) gfx_put_pixel(x, y, COLOR_BLACK);
            else gfx_put_pixel(x, y, COLOR_WHITE);
        }
    }
}

static void desktop_render(void) {
    gfx_clear(COLOR_BLUE);
    /* Simple patterned wallpaper. */
    for (int y = 24; y < GFX_HEIGHT - TASKBAR_HEIGHT; y += 16)
        for (int x = (y / 16) % 2 ? 8 : 16; x < GFX_WIDTH; x += 32)
            gfx_put_pixel(x, y, COLOR_LIGHT_BLUE);

    /* Draw unfocused window first, focused window last. */
    draw_window(1 - focused_window);
    draw_window(focused_window);

    gfx_fill_rect(0, GFX_HEIGHT - TASKBAR_HEIGHT, GFX_WIDTH, TASKBAR_HEIGHT, COLOR_DARK_GRAY);
    gfx_draw_rect(3, GFX_HEIGHT - 14, 44, 12, COLOR_WHITE, COLOR_LIGHT_BLUE);
    gfx_draw_string(8, GFX_HEIGHT - 11, "START", COLOR_WHITE, COLOR_LIGHT_BLUE);
    gfx_draw_string(55, GFX_HEIGHT - 11, "TAB FOCUS  ARROWS MOVE  X HIDE", COLOR_WHITE, COLOR_DARK_GRAY);
    draw_cursor();
}

static void focus_next(void) {
    focused_window = 1 - focused_window;
    if (!windows[focused_window].visible) windows[focused_window].visible = 1;
}

static void handle_mouse(const input_event_t *event) {
    cursor_x = clamp(cursor_x + event->mouse_dx, 0, GFX_WIDTH - 1);
    cursor_y = clamp(cursor_y + event->mouse_dy, 0, GFX_HEIGHT - TASKBAR_HEIGHT - 1);
    int left_down = (event->mouse_buttons & 1) != 0;
    int left_pressed = left_down && !(previous_buttons & 1);

    if (left_pressed) {
        for (int i = WINDOW_COUNT - 1; i >= 0; --i) {
            window_t *w = &windows[i];
            if (w->visible && cursor_x >= w->x && cursor_x < w->x + w->w &&
                cursor_y >= w->y && cursor_y < w->y + 15) {
                focused_window = i;
                dragging = 1;
                drag_dx = cursor_x - w->x;
                drag_dy = cursor_y - w->y;
                break;
            }
        }
    }

    if (!left_down) dragging = 0;
    if (dragging && left_down) {
        window_t *w = &windows[focused_window];
        w->x = clamp(cursor_x - drag_dx, 0, GFX_WIDTH - w->w);
        w->y = clamp(cursor_y - drag_dy, 18, GFX_HEIGHT - TASKBAR_HEIGHT - w->h);
    }
    previous_buttons = event->mouse_buttons;
}

void desktop_init(void) {
    focused_window = 0;
    cursor_x = GFX_WIDTH / 2;
    cursor_y = GFX_HEIGHT / 2;
    dragging = 0;
    previous_buttons = 0;
    input_init();
    desktop_render();
}

void desktop_run(void) {
    input_event_t event;
    for (;;) {
        if (!input_poll(&event)) continue;
        if (event.type == INPUT_MOUSE) {
            handle_mouse(&event);
        } else if (event.type == INPUT_KEY) {
            switch (event.key) {
                case KEY_UP: cursor_y = clamp(cursor_y - CURSOR_STEP, 0, GFX_HEIGHT - TASKBAR_HEIGHT - 1); break;
                case KEY_DOWN: cursor_y = clamp(cursor_y + CURSOR_STEP, 0, GFX_HEIGHT - TASKBAR_HEIGHT - 1); break;
                case KEY_LEFT: cursor_x = clamp(cursor_x - CURSOR_STEP, 0, GFX_WIDTH - 1); break;
                case KEY_RIGHT: cursor_x = clamp(cursor_x + CURSOR_STEP, 0, GFX_WIDTH - 1); break;
                case KEY_TAB: focus_next(); break;
                case KEY_ENTER: windows[focused_window].visible = 1; break;
                case KEY_CLOSE: windows[focused_window].visible = !windows[focused_window].visible; focus_next(); break;
                default: break;
            }
        }
        desktop_render();
    }
}
