#include "desktop.h"
#include "graphics.h"
#include "input.h"
#include "ports.h"

#define WINDOW_COUNT 4
#define TASKBAR_HEIGHT 16
#define TERMINAL_ROWS 8
#define TERMINAL_COLS 45
#define COMMAND_MAX 46

typedef enum { APP_FILES, APP_INFO, APP_CLOCK, APP_TERMINAL } app_id_t;
typedef struct {
    int x, y, w, h;
    const char *title;
    uint8_t color;
    int visible;
} window_t;

static window_t windows[WINDOW_COUNT] = {
    {12, 34, 144, 92, "FILES", COLOR_LIGHT_RED, 1},
    {84, 48, 150, 78, "SYSTEM INFO", COLOR_LIGHT_GREEN, 1},
    {76, 48, 156, 72, "CLOCK", COLOR_LIGHT_CYAN, 0},
    {8, 24, 304, 156, "TERMINAL", COLOR_LIGHT_BLUE, 0}
};
static int z_order[WINDOW_COUNT] = {0, 1, 2, 3};
static int focused;
static int cursor_x = GFX_WIDTH / 2, cursor_y = GFX_HEIGHT / 2;
static int dragging, drag_dx, drag_dy;
static uint8_t previous_buttons;
static int start_open;
static char terminal_lines[TERMINAL_ROWS][TERMINAL_COLS + 1];
static char command[COMMAND_MAX];
static unsigned int command_length;
static unsigned int terminal_line_count;
static unsigned char clock_second = 0xFF;

static int clamp(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

static void bring_to_front(int app) {
    int pos = 0;
    while (pos < WINDOW_COUNT && z_order[pos] != app) ++pos;
    if (pos == WINDOW_COUNT) return;
    for (int i = pos; i < WINDOW_COUNT - 1; ++i) z_order[i] = z_order[i + 1];
    z_order[WINDOW_COUNT - 1] = app;
    focused = app;
}

static void terminal_add_line(const char *text) {
    for (int i = 0; i < TERMINAL_ROWS - 1; ++i) {
        for (int j = 0; j <= TERMINAL_COLS; ++j) terminal_lines[i][j] = terminal_lines[i + 1][j];
    }
    unsigned int i = 0;
    while (text[i] && i < TERMINAL_COLS) { terminal_lines[TERMINAL_ROWS - 1][i] = text[i]; ++i; }
    terminal_lines[TERMINAL_ROWS - 1][i] = '\0';
    ++terminal_line_count;
}

static void terminal_execute(void) {
    command[command_length] = '\0';
    if (command_length == 0) { terminal_add_line(" "); return; }
    terminal_add_line(command);
    if (command[0] == 'h' || command[0] == 'H') terminal_add_line("HELP ECHO CLEAR LS DATE ABOUT");
    else if ((command[0] == 'e' || command[0] == 'E') && (command[1] == 'c' || command[1] == 'C')) {
        unsigned int i = 4;
        while (command[i] == ' ') ++i;
        terminal_add_line(command[i] ? command + i : "USAGE: ECHO TEXT");
    } else if (command[0] == 'c' || command[0] == 'C') {
        for (int i = 0; i < TERMINAL_ROWS; ++i) terminal_lines[i][0] = '\0';
    } else if (command[0] == 'l' || command[0] == 'L') terminal_add_line("DESKTOP  TERMINAL  README.TXT");
    else if (command[0] == 'a' || command[0] == 'A') terminal_add_line("SUPERNOVA GUI TERMINAL DEMO");
    else if (command[0] == 'd' || command[0] == 'D') terminal_add_line("CLOCK APP SHOWS CMOS RTC TIME");
    else terminal_add_line("UNKNOWN COMMAND - TYPE HELP");
    command_length = 0;
    command[0] = '\0';
}

static void draw_terminal(window_t *w) {
    gfx_draw_string(w->x + 8, w->y + 22, "NOVA GUI TERMINAL - HELP FOR COMMANDS", COLOR_LIGHT_GREEN, COLOR_BLACK);
    for (int i = 0; i < TERMINAL_ROWS; ++i)
        gfx_draw_string(w->x + 8, w->y + 34 + i * 9, terminal_lines[i], COLOR_WHITE, COLOR_BLACK);
    gfx_draw_string(w->x + 8, w->y + 34 + TERMINAL_ROWS * 9, ">", COLOR_YELLOW, COLOR_BLACK);
    gfx_draw_string(w->x + 16, w->y + 34 + TERMINAL_ROWS * 9, command, COLOR_WHITE, COLOR_BLACK);
    if ((terminal_line_count & 1u) == 0) gfx_fill_rect(w->x + 18 + (int)command_length * 6, w->y + 33 + TERMINAL_ROWS * 9, 4, 8, COLOR_WHITE);
}

static unsigned char cmos_read(uint8_t reg) {
    port_byte_out(0x70, (uint8_t)(reg | 0x80));
    return port_byte_in(0x71);
}

static unsigned char bcd_to_bin(unsigned char v) { return (unsigned char)((v & 15) + (v >> 4) * 10); }

static void draw_clock(window_t *w) {
    unsigned char status_b = cmos_read(0x0B);
    unsigned char h = cmos_read(0x04), m = cmos_read(0x02), s = cmos_read(0x00);
    int pm = (h & 0x80) != 0;
    h &= 0x7F;
    if (!(status_b & 0x04)) { h = bcd_to_bin(h); m = bcd_to_bin(m); s = bcd_to_bin(s); }
    if (!(status_b & 0x02)) { h = (unsigned char)(h % 12); if (pm) h += 12; }
    clock_second = s;
    char time[9] = { (char)('0' + h / 10), (char)('0' + h % 10), ':',
                     (char)('0' + m / 10), (char)('0' + m % 10), ':',
                     (char)('0' + s / 10), (char)('0' + s % 10), '\0' };
    gfx_draw_string(w->x + 36, w->y + 34, time, COLOR_WHITE, COLOR_BLACK);
    gfx_draw_string(w->x + 22, w->y + 50, "QEMU CMOS REAL TIME", COLOR_LIGHT_CYAN, COLOR_BLACK);
}

static void draw_window(int app) {
    window_t *w = &windows[app];
    if (!w->visible) return;
    uint8_t border = app == focused ? COLOR_YELLOW : COLOR_WHITE;
    gfx_draw_rect(w->x, w->y, w->w, w->h, border, COLOR_DARK_GRAY);
    gfx_fill_rect(w->x + 1, w->y + 1, w->w - 2, 13, w->color);
    gfx_draw_string(w->x + 5, w->y + 4, w->title, COLOR_WHITE, w->color);
    gfx_fill_rect(w->x + w->w - 20, w->y + 2, 7, 9, COLOR_DARK_GRAY);
    gfx_draw_string(w->x + w->w - 19, w->y + 3, "-", COLOR_WHITE, COLOR_DARK_GRAY);
    gfx_fill_rect(w->x + w->w - 11, w->y + 2, 8, 9, COLOR_RED);
    gfx_draw_string(w->x + w->w - 10, w->y + 3, "X", COLOR_WHITE, COLOR_RED);
    gfx_fill_rect(w->x + 3, w->y + 17, w->w - 6, w->h - 20, COLOR_BLACK);
    if (app == APP_FILES) {
        gfx_draw_string(w->x + 8, w->y + 24, "IN MEMORY DEMO", COLOR_YELLOW, COLOR_BLACK);
        gfx_draw_string(w->x + 8, w->y + 37, "DESKTOP", COLOR_WHITE, COLOR_BLACK);
        gfx_draw_string(w->x + 8, w->y + 50, "TERMINAL", COLOR_WHITE, COLOR_BLACK);
        gfx_draw_string(w->x + 8, w->y + 63, "README TXT", COLOR_WHITE, COLOR_BLACK);
    } else if (app == APP_INFO) {
        gfx_draw_string(w->x + 8, w->y + 25, "SUPERNOVA OS", COLOR_WHITE, COLOR_BLACK);
        gfx_draw_string(w->x + 8, w->y + 38, "VGA 320 X 200", COLOR_LIGHT_CYAN, COLOR_BLACK);
        gfx_draw_string(w->x + 8, w->y + 51, "KEYS: TAB ARROWS", COLOR_WHITE, COLOR_BLACK);
    } else if (app == APP_CLOCK) draw_clock(w);
    else draw_terminal(w);
}

static void draw_cursor(void) {
    for (int row = 0; row < 10; ++row) {
        for (int col = 0; col <= row / 2; ++col) {
            int x = cursor_x + col, y = cursor_y + row;
            if (row == 9 || col == 0 || col == row / 2) gfx_put_pixel(x, y, COLOR_BLACK);
            else gfx_put_pixel(x, y, COLOR_WHITE);
        }
    }
}

static void draw_start_menu(void) {
    if (!start_open) return;
    gfx_draw_rect(4, 92, 102, 88, COLOR_WHITE, COLOR_DARK_GRAY);
    gfx_draw_string(12, 99, "OPEN APP", COLOR_YELLOW, COLOR_DARK_GRAY);
    gfx_draw_string(12, 114, "FILES", COLOR_WHITE, COLOR_DARK_GRAY);
    gfx_draw_string(12, 128, "TERMINAL", COLOR_WHITE, COLOR_DARK_GRAY);
    gfx_draw_string(12, 142, "INFO", COLOR_WHITE, COLOR_DARK_GRAY);
    gfx_draw_string(12, 156, "CLOCK", COLOR_WHITE, COLOR_DARK_GRAY);
}

static void desktop_render(void) {
    gfx_clear(COLOR_BLUE);
    for (int y = 20; y < GFX_HEIGHT - TASKBAR_HEIGHT; y += 16)
        for (int x = (y / 16) % 2 ? 8 : 16; x < GFX_WIDTH; x += 32)
            gfx_put_pixel(x, y, COLOR_LIGHT_BLUE);
    for (int i = 0; i < WINDOW_COUNT; ++i) draw_window(z_order[i]);
    gfx_fill_rect(0, GFX_HEIGHT - TASKBAR_HEIGHT, GFX_WIDTH, TASKBAR_HEIGHT, COLOR_DARK_GRAY);
    gfx_draw_rect(3, GFX_HEIGHT - 14, 44, 12, COLOR_WHITE, COLOR_LIGHT_BLUE);
    gfx_draw_string(8, GFX_HEIGHT - 11, "START", COLOR_WHITE, COLOR_LIGHT_BLUE);
    for (int i = 0; i < WINDOW_COUNT; ++i) {
        int app = z_order[i];
        if (windows[app].visible) {
            int bx = 51 + i * 52;
            gfx_fill_rect(bx, GFX_HEIGHT - 13, 48, 10, app == focused ? COLOR_LIGHT_BLUE : COLOR_DARK_GRAY);
            gfx_draw_string(bx + 3, GFX_HEIGHT - 11, windows[app].title, COLOR_WHITE,
                            app == focused ? COLOR_LIGHT_BLUE : COLOR_DARK_GRAY);
        }
    }
    draw_start_menu();
    draw_cursor();
    gfx_present();
}

static void open_app(int app) {
    windows[app].visible = 1;
    bring_to_front(app);
    start_open = 0;
}

static void focus_next(void) {
    int next = (focused + 1) % WINDOW_COUNT;
    for (int n = 0; n < WINDOW_COUNT; ++n) {
        int candidate = (next + n) % WINDOW_COUNT;
        if (windows[candidate].visible) { bring_to_front(candidate); return; }
    }
}

static int point_in(int x, int y, int rx, int ry, int rw, int rh) {
    return x >= rx && x < rx + rw && y >= ry && y < ry + rh;
}

static void handle_click(void) {
    if (point_in(cursor_x, cursor_y, 3, GFX_HEIGHT - 14, 44, 12)) { start_open = !start_open; return; }
    if (start_open) {
        if (point_in(cursor_x, cursor_y, 4, 108, 102, 14)) open_app(APP_FILES);
        else if (point_in(cursor_x, cursor_y, 4, 122, 102, 14)) open_app(APP_TERMINAL);
        else if (point_in(cursor_x, cursor_y, 4, 136, 102, 14)) open_app(APP_INFO);
        else if (point_in(cursor_x, cursor_y, 4, 150, 102, 14)) open_app(APP_CLOCK);
        else start_open = 0;
        return;
    }
    /* Click taskbar app tabs to restore/focus them. */
    for (int i = 0; i < WINDOW_COUNT; ++i) {
        int app = z_order[i], bx = 51 + i * 52;
        if (windows[app].visible && point_in(cursor_x, cursor_y, bx, GFX_HEIGHT - 15, 48, 14)) {
            bring_to_front(app); return;
        }
    }
    /* Hit test from topmost to bottommost. */
    for (int pos = WINDOW_COUNT - 1; pos >= 0; --pos) {
        int app = z_order[pos]; window_t *w = &windows[app];
        if (!w->visible || !point_in(cursor_x, cursor_y, w->x, w->y, w->w, 15)) continue;
        bring_to_front(app);
        if (cursor_x >= w->x + w->w - 12) { w->visible = 0; focus_next(); return; }
        if (cursor_x >= w->x + w->w - 22) { w->visible = 0; return; }
        dragging = 1; drag_dx = cursor_x - w->x; drag_dy = cursor_y - w->y;
        return;
    }
    /* File-list click opens terminal for the named terminal entry. */
    window_t *files = &windows[APP_FILES];
    if (files->visible && point_in(cursor_x, cursor_y, files->x + 4, files->y + 45, files->w - 8, 13)) open_app(APP_TERMINAL);
}

static void handle_mouse(const input_event_t *event) {
    cursor_x = clamp(cursor_x + event->mouse_dx, 0, GFX_WIDTH - 1);
    cursor_y = clamp(cursor_y + event->mouse_dy, 0, GFX_HEIGHT - TASKBAR_HEIGHT - 1);
    int down = (event->mouse_buttons & 1) != 0;
    if (down && !(previous_buttons & 1)) handle_click();
    if (!down) dragging = 0;
    if (dragging && down) {
        window_t *w = &windows[focused];
        w->x = clamp(cursor_x - drag_dx, 0, GFX_WIDTH - w->w);
        w->y = clamp(cursor_y - drag_dy, 18, GFX_HEIGHT - TASKBAR_HEIGHT - w->h);
    }
    previous_buttons = event->mouse_buttons;
}

static void handle_key(const input_event_t *event) {
    if (event->key == KEY_TAB) { focus_next(); return; }
    if (event->key == KEY_UP) { cursor_y = clamp(cursor_y - 4, 0, GFX_HEIGHT - TASKBAR_HEIGHT - 1); return; }
    if (event->key == KEY_DOWN) { cursor_y = clamp(cursor_y + 4, 0, GFX_HEIGHT - TASKBAR_HEIGHT - 1); return; }
    if (event->key == KEY_LEFT) { cursor_x = clamp(cursor_x - 4, 0, GFX_WIDTH - 1); return; }
    if (event->key == KEY_RIGHT) { cursor_x = clamp(cursor_x + 4, 0, GFX_WIDTH - 1); return; }
    if (event->key == KEY_CLOSE) { windows[focused].visible = 0; focus_next(); return; }
    if (focused != APP_TERMINAL) return;
    if (event->key == KEY_BACKSPACE) {
        if (command_length) command[--command_length] = '\0';
    } else if (event->key == KEY_ENTER) terminal_execute();
    else if (event->character >= 32 && event->character <= 126 && command_length < COMMAND_MAX - 1) {
        command[command_length++] = event->character;
        command[command_length] = '\0';
    }
}

void desktop_init(void) {
    focused = APP_FILES; start_open = 0; dragging = 0; previous_buttons = 0;
    cursor_x = GFX_WIDTH / 2; cursor_y = GFX_HEIGHT / 2;
    command_length = terminal_line_count = 0; command[0] = '\0';
    for (int i = 0; i < TERMINAL_ROWS; ++i) terminal_lines[i][0] = '\0';
    terminal_add_line("WELCOME TO SUPERNOVA");
    terminal_add_line("TYPE HELP TO BEGIN");
    input_init();
    desktop_render();
}

void desktop_run(void) {
    input_event_t event;
    for (;;) {
        int changed = 0;
        if (input_poll(&event)) {
            if (event.type == INPUT_MOUSE) handle_mouse(&event);
            else if (event.type == INPUT_KEY) handle_key(&event);
            changed = 1;
        }
        if (windows[APP_CLOCK].visible) {
            unsigned char second = cmos_read(0x00);
            if (second != clock_second) changed = 1;
        }
        if (changed) desktop_render();
    }
}
