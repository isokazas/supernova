

#include <stdint.h>
#include <stddef.h>

/* ============================================================================
 *  SECTION 1: Low-level port I/O (needed for RTC clock + reboot trick)
 * ==========================================================================*/

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

/* ============================================================================
 *  SECTION 2: VGA text-mode output driver (replace term_putc if you already
 *  have your own console driver — nothing else in this file needs changing)
 * ==========================================================================*/

#define VGA_WIDTH   80
#define VGA_HEIGHT  25
#define VGA_MEMORY  ((volatile uint16_t*)0xB8000)
#define VGA_COLOR   0x0F /* white on black */

static size_t term_row = 0;
static size_t term_col = 0;

static void vga_scroll(void) {
    volatile uint16_t *vga = VGA_MEMORY;
    for (size_t y = 1; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            vga[(y - 1) * VGA_WIDTH + x] = vga[y * VGA_WIDTH + x];
        }
    }
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        vga[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = (uint16_t)(' ' | (VGA_COLOR << 8));
    }
    term_row = VGA_HEIGHT - 1;
}

static void term_putc(char c) {
    volatile uint16_t *vga = VGA_MEMORY;

    if (c == '\n') {
        term_col = 0;
        term_row++;
    } else if (c == '\r') {
        term_col = 0;
    } else if (c == '\b') {
        if (term_col > 0) term_col--;
        vga[term_row * VGA_WIDTH + term_col] = (uint16_t)(' ' | (VGA_COLOR << 8));
    } else {
        vga[term_row * VGA_WIDTH + term_col] = (uint16_t)((unsigned char)c | (VGA_COLOR << 8));
        term_col++;
    }

    if (term_col >= VGA_WIDTH) {
        term_col = 0;
        term_row++;
    }
    if (term_row >= VGA_HEIGHT) {
        vga_scroll();
    }
}

static void term_print(const char *s) {
    while (*s) term_putc(*s++);
}

static void term_clear(void) {
    volatile uint16_t *vga = VGA_MEMORY;
    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga[i] = (uint16_t)(' ' | (VGA_COLOR << 8));
    }
    term_row = 0;
    term_col = 0;
}

/* ============================================================================
 *  SECTION 3: Minimal freestanding string/memory helpers (no libc available)
 * ==========================================================================*/

static size_t k_strlen(const char *s) {
    size_t n = 0;
    while (s[n]) n++;
    return n;
}

static int k_strcmp(const char *a, const char *b) {
    while (*a && (*a == *b)) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

static void k_strcpy(char *dst, const char *src, size_t max) {
    size_t i = 0;
    for (; i < max - 1 && src[i]; i++) dst[i] = src[i];
    dst[i] = '\0';
}

static void k_memset(void *dst, int val, size_t n) {
    unsigned char *p = (unsigned char*)dst;
    for (size_t i = 0; i < n; i++) p[i] = (unsigned char)val;
}

static void k_itoa(int value, char *buf, int base) {
    char tmp[32];
    int i = 0, neg = 0;
    unsigned int uval;

    if (value < 0 && base == 10) { neg = 1; uval = (unsigned int)(-value); }
    else uval = (unsigned int)value;

    if (uval == 0) tmp[i++] = '0';
    while (uval) {
        int d = uval % base;
        tmp[i++] = (char)(d < 10 ? '0' + d : 'a' + (d - 10));
        uval /= base;
    }
    if (neg) tmp[i++] = '-';

    int j = 0;
    while (i > 0) buf[j++] = tmp[--i];
    buf[j] = '\0';
}

/* ============================================================================
 *  SECTION 4: Tiny in-memory fake filesystem (flat namespace, RAM-backed)
 * ==========================================================================*/

#define MAX_FILES     32
#define MAX_NAME_LEN  32
#define MAX_CONTENT   256

typedef struct {
    char name[MAX_NAME_LEN];
    char content[MAX_CONTENT];
    int used;
    int is_dir;
} fake_file_t;

static fake_file_t fs_table[MAX_FILES];
static char current_dir[MAX_NAME_LEN] = "";  /* "" = drive root */

#define DRIVE_PREFIX "A://"

/* Prints the current location Windows-drive style: "A://" at root,
 * "A://hi/" inside folder "hi", "A://hi/there/" if nested, etc. */
static void print_current_path(void) {
    term_print(DRIVE_PREFIX);
    if (current_dir[0]) {
        term_print(current_dir);
        term_print("/");
    }
}

static fake_file_t* fs_find(const char *name) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (fs_table[i].used && k_strcmp(fs_table[i].name, name) == 0) {
            return &fs_table[i];
        }
    }
    return NULL;
}

static fake_file_t* fs_create(const char *name, int is_dir) {
    if (fs_find(name)) return NULL; /* already exists */
        for (int i = 0; i < MAX_FILES; i++) {
            if (!fs_table[i].used) {
                fs_table[i].used = 1;
                fs_table[i].is_dir = is_dir;
                fs_table[i].content[0] = '\0';
                k_strcpy(fs_table[i].name, name, MAX_NAME_LEN);
                return &fs_table[i];
            }
        }
        return NULL; /* full */
}

static int fs_delete(const char *name) {
    fake_file_t *f = fs_find(name);
    if (!f) return -1;
    f->used = 0;
    return 0;
}

/* ============================================================================
 *  SECTION 5: Fake process table (no real multitasking here — just a demo
 *  view, wire this up to your real scheduler's task list if you have one)
 * ==========================================================================*/

typedef struct {
    int pid;
    const char *name;
    const char *state;
} fake_proc_t;

static fake_proc_t fake_procs[] = {
    {0, "kernel_main",  "running"},
    {1, "terminal.c",   "running"},
    {2, "idle_loop",    "sleeping"},
};
#define FAKE_PROC_COUNT (sizeof(fake_procs) / sizeof(fake_procs[0]))

/* ============================================================================
 *  SECTION 6: Command history
 * ==========================================================================*/

#define HISTORY_SIZE 16
#define LINE_MAX     128

static char history[HISTORY_SIZE][LINE_MAX];
static int history_count = 0;
static int history_next  = 0;

static void history_add(const char *line) {
    k_strcpy(history[history_next], line, LINE_MAX);
    history_next = (history_next + 1) % HISTORY_SIZE;
    if (history_count < HISTORY_SIZE) history_count++;
}

/* ============================================================================
 *  SECTION 7: Command handlers
 *  (Each Linux-equivalent is noted in a comment next to its renamed command)
 * ==========================================================================*/

#define MAX_ARGS 8

static void cmd_help(int argc, char **argv);   /* forward-declared, needs table */

/* --- clr  (Linux: clear) --- */
static void cmd_clr(int argc, char **argv) {
    (void)argc; (void)argv;
    term_clear();
}

/* --- say  (Linux: echo) --- */
static void cmd_say(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        term_print(argv[i]);
        if (i != argc - 1) term_putc(' ');
    }
    term_putc('\n');
}

/* --- dir  (Linux: ls) --- */
static void cmd_dir(int argc, char **argv) {
    (void)argc; (void)argv;
    int found = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        if (fs_table[i].used) {
            term_print(fs_table[i].is_dir ? "[dir]  " : "[file] ");
            term_print(fs_table[i].name);
            term_putc('\n');
            found = 1;
        }
    }
    if (!found) term_print("(empty)\n");
}

/* --- show  (Linux: cat) --- */
static void cmd_show(int argc, char **argv) {
    if (argc < 2) { term_print("usage: show <filename>\n"); return; }
    fake_file_t *f = fs_find(argv[1]);
    if (!f) { term_print("show: no such file\n"); return; }
    if (f->is_dir) { term_print("show: is a folder\n"); return; }
    term_print(f->content);
    term_putc('\n');
}

/* --- mkfile  (Linux: touch, with optional inline content) --- */
static void cmd_mkfile(int argc, char **argv) {
    if (argc < 2) { term_print("usage: mkfile <filename> [content...]\n"); return; }
    fake_file_t *f = fs_create(argv[1], 0);
    if (!f) { term_print("mkfile: exists already or table full\n"); return; }
    f->content[0] = '\0';
    for (int i = 2; i < argc; i++) {
        size_t len = k_strlen(f->content);
        size_t alen = k_strlen(argv[i]);
        if (len + alen + 2 < MAX_CONTENT) {
            if (i > 2) f->content[len++] = ' ';
            k_strcpy(f->content + len, argv[i], MAX_CONTENT - len);
        }
    }
    term_print("created: ");
    term_print(argv[1]);
    term_putc('\n');
}

/* --- rmfile  (Linux: rm) --- */
static void cmd_rmfile(int argc, char **argv) {
    if (argc < 2) { term_print("usage: rmfile <filename>\n"); return; }
    if (fs_delete(argv[1]) == 0) {
        term_print("removed: ");
        term_print(argv[1]);
        term_putc('\n');
    } else {
        term_print("rmfile: no such file\n");
    }
}

/* --- mkfolder  (Linux: mkdir) --- */
static void cmd_mkfolder(int argc, char **argv) {
    if (argc < 2) { term_print("usage: mkfolder <name>\n"); return; }
    if (fs_create(argv[1], 1)) {
        term_print("folder created: ");
        term_print(argv[1]);
        term_putc('\n');
    } else {
        term_print("mkfolder: exists already or table full\n");
    }
}

/* --- rmfolder  (Linux: rmdir) --- */
static void cmd_rmfolder(int argc, char **argv) {
    if (argc < 2) { term_print("usage: rmfolder <name>\n"); return; }
    fake_file_t *f = fs_find(argv[1]);
    if (!f || !f->is_dir) { term_print("rmfolder: no such folder\n"); return; }
    fs_delete(argv[1]);
    term_print("folder removed: ");
    term_print(argv[1]);
    term_putc('\n');
}

/* --- goto  (Linux: cd) --- */
static void cmd_goto(int argc, char **argv) {
    if (argc < 2) { print_current_path(); term_putc('\n'); return; }

    /* goto / or goto ~  -> back to drive root */
    if (k_strcmp(argv[1], "/") == 0 || k_strcmp(argv[1], "~") == 0) {
        current_dir[0] = '\0';
        print_current_path(); term_putc('\n');
        return;
    }

    /* goto ..  -> step up one folder */
    if (k_strcmp(argv[1], "..") == 0) {
        if (current_dir[0] == '\0') { term_print("goto: already at root\n"); return; }
        int i = (int)k_strlen(current_dir) - 1;
        while (i > 0 && current_dir[i] != '/') i--;
        current_dir[i] = '\0';
        print_current_path(); term_putc('\n');
        return;
    }

    /* otherwise the target must be a real folder in the fake fs */
    fake_file_t *f = fs_find(argv[1]);
    if (!f || !f->is_dir) { term_print("goto: no such folder\n"); return; }

    if (current_dir[0]) {
        size_t len = k_strlen(current_dir);
        current_dir[len] = '/';
        k_strcpy(current_dir + len + 1, argv[1], MAX_NAME_LEN - len - 1);
    } else {
        k_strcpy(current_dir, argv[1], MAX_NAME_LEN);
    }

    print_current_path();
    term_putc('\n');
}

/* --- whereami  (Linux: pwd) --- */
static void cmd_whereami(int argc, char **argv) {
    (void)argc; (void)argv;
    print_current_path();
    term_putc('\n');
}

/* --- me  (Linux: whoami) --- */
static void cmd_me(int argc, char **argv) {
    (void)argc; (void)argv;
    term_print("root\n");
}

/* --- tasks  (Linux: ps) --- */
static void cmd_tasks(int argc, char **argv) {
    (void)argc; (void)argv;
    char buf[16];
    term_print("PID  STATE      NAME\n");
    for (size_t i = 0; i < FAKE_PROC_COUNT; i++) {
        k_itoa(fake_procs[i].pid, buf, 10);
        term_print(buf);
        term_print("    ");
        term_print(fake_procs[i].state);
        term_print("   ");
        term_print(fake_procs[i].name);
        term_putc('\n');
    }
}

/* --- sysinfo  (Linux: uname) --- */
static void cmd_sysinfo(int argc, char **argv) {
    (void)argc; (void)argv;
    term_print("NovaShell Custom OS  |  kernel: dev-build  |  arch: x86\n");
}

/* --- meminfo  (Linux: free) --- */
static void cmd_meminfo(int argc, char **argv) {
    (void)argc; (void)argv;
    int used = 0;
    for (int i = 0; i < MAX_FILES; i++) if (fs_table[i].used) used++;
    char buf[16];
    term_print("fake-fs slots used: ");
    k_itoa(used, buf, 10);
    term_print(buf);
    term_print(" / ");
    k_itoa(MAX_FILES, buf, 10);
    term_print(buf);
    term_putc('\n');
    term_print("(hook this up to your real physical memory allocator for live stats)\n");
}

/* --- now  (Linux: date) — reads the CMOS RTC directly via I/O ports --- */
static uint8_t cmos_read(uint8_t reg) {
    outb(0x70, reg);
    return inb(0x71);
}

static void cmd_now(int argc, char **argv) {
    (void)argc; (void)argv;
    /* CMOS RTC values are BCD-encoded by default on most hardware */
    uint8_t sec  = cmos_read(0x00);
    uint8_t min  = cmos_read(0x02);
    uint8_t hour = cmos_read(0x04);
    uint8_t day  = cmos_read(0x07);
    uint8_t mon  = cmos_read(0x08);
    uint8_t year = cmos_read(0x09);

    #define BCD2BIN(v) ((v & 0x0F) + ((v / 16) * 10))
    sec = BCD2BIN(sec); min = BCD2BIN(min); hour = BCD2BIN(hour);
    day = BCD2BIN(day); mon = BCD2BIN(mon); year = BCD2BIN(year);
    #undef BCD2BIN

    char buf[8];
    k_itoa(20, buf, 10); term_print(buf);
    k_itoa(year, buf, 10); if (year < 10) term_print("0"); term_print(buf); term_print("-");
    k_itoa(mon, buf, 10);  if (mon  < 10) term_print("0"); term_print(buf); term_print("-");
    k_itoa(day, buf, 10);  if (day  < 10) term_print("0"); term_print(buf); term_print(" ");
    k_itoa(hour, buf, 10); if (hour < 10) term_print("0"); term_print(buf); term_print(":");
    k_itoa(min, buf, 10);  if (min  < 10) term_print("0"); term_print(buf); term_print(":");
    k_itoa(sec, buf, 10);  if (sec  < 10) term_print("0"); term_print(buf);
    term_print(" (UTC, unadjusted for RTC century register)\n");
}

/* --- past  (Linux: history) --- */
static void cmd_past(int argc, char **argv) {
    (void)argc; (void)argv;
    int start = (history_next - history_count + HISTORY_SIZE) % HISTORY_SIZE;
    for (int i = 0; i < history_count; i++) {
        int idx = (start + i) % HISTORY_SIZE;
        term_print(history[idx]);
        term_putc('\n');
    }
}

/* --- calc  (bonus: Linux "expr", simple two-operand integer calculator) --- */
static void cmd_calc(int argc, char **argv) {
    if (argc != 4) { term_print("usage: calc <a> <+|-|*|/> <b>\n"); return; }
    int a = 0, b = 0, sign_a = 1, sign_b = 1, i = 0;
    char *pa = argv[1], *pb = argv[3];
    if (*pa == '-') { sign_a = -1; pa++; }
    if (*pb == '-') { sign_b = -1; pb++; }
    for (i = 0; pa[i]; i++) a = a * 10 + (pa[i] - '0');
    for (i = 0; pb[i]; i++) b = b * 10 + (pb[i] - '0');
    a *= sign_a; b *= sign_b;

    int result = 0;
    char op = argv[2][0];
    if (op == '+') result = a + b;
    else if (op == '-') result = a - b;
    else if (op == '*') result = a * b;
    else if (op == '/') { if (b == 0) { term_print("calc: divide by zero\n"); return; } result = a / b; }
    else { term_print("calc: unknown operator\n"); return; }

    char buf[16];
    k_itoa(result, buf, 10);
    term_print(buf);
    term_putc('\n');
}

/* --- ver  (Linux: uname -a / lsb_release, simplified) --- */
static void cmd_ver(int argc, char **argv) {
    (void)argc; (void)argv;
    term_print("NovaShell v0.1 - single-file freestanding terminal\n");
}

/* --- halt  (Linux: poweroff/halt) --- */
static void cmd_halt(int argc, char **argv) {
    (void)argc; (void)argv;
    term_print("System halted.\n");
    __asm__ volatile ("cli");
    for (;;) { __asm__ volatile ("hlt"); }
}

/* --- reboot  (Linux: reboot) — classic 8042 keyboard-controller pulse --- */
static void cmd_reboot(int argc, char **argv) {
    (void)argc; (void)argv;
    term_print("Rebooting...\n");
    uint8_t good = 0x02;
    while (good & 0x02) good = inb(0x64);
    outb(0x64, 0xFE);
    for (;;) { __asm__ volatile ("hlt"); } /* fallback if reboot didn't take */
}

/* ============================================================================
 *  SECTION 8: Command table (help is generated FROM this, so it never drifts)
 * ==========================================================================*/

typedef struct {
    const char *name;
    const char *linux_equiv;
    const char *description;
    void (*handler)(int argc, char **argv);
} command_t;

static const command_t commands[] = {
    {"help",     "help/man",  "list all available commands",              cmd_help},
    {"clr",      "clear",     "clear the screen",                         cmd_clr},
    {"say",      "echo",      "print the given text back to the screen",  cmd_say},
    {"dir",      "ls",        "list files and folders",                   cmd_dir},
    {"show",     "cat",       "print a file's contents",                  cmd_show},
    {"mkfile",   "touch",     "create a file, optionally with content",   cmd_mkfile},
    {"rmfile",   "rm",        "delete a file",                            cmd_rmfile},
    {"mkfolder", "mkdir",     "create a folder",                          cmd_mkfolder},
    {"rmfolder", "rmdir",     "delete a folder",                          cmd_rmfolder},
    {"goto",     "cd",        "change the current directory",             cmd_goto},
    {"whereami", "pwd",       "print the current directory",              cmd_whereami},
    {"me",       "whoami",    "print the current user",                   cmd_me},
    {"tasks",    "ps",        "list running processes",                   cmd_tasks},
    {"sysinfo",  "uname",     "print system information",                 cmd_sysinfo},
    {"meminfo",  "free",      "print fake-fs / memory usage",             cmd_meminfo},
    {"now",      "date",      "print the current date and time",          cmd_now},
    {"past",     "history",   "print command history",                    cmd_past},
    {"calc",     "expr",      "simple integer calculator: calc 2 + 2",    cmd_calc},
    {"ver",      "uname -a",  "print shell version",                      cmd_ver},
    {"reboot",   "reboot",    "reboot the machine",                       cmd_reboot},
    {"halt",     "halt",      "halt the CPU",                             cmd_halt},
};
#define COMMAND_COUNT (sizeof(commands) / sizeof(commands[0]))

static void cmd_help(int argc, char **argv) {
    (void)argc; (void)argv;
    term_print("Available commands (renamed-Linux-equivalent shown in brackets):\n\n");
    for (size_t i = 0; i < COMMAND_COUNT; i++) {
        term_print("  ");
        term_print(commands[i].name);
        /* pad to column 12 for alignment */
        size_t len = k_strlen(commands[i].name);
        for (size_t p = len; p < 10; p++) term_putc(' ');
        term_print("[");
        term_print(commands[i].linux_equiv);
        term_print("] - ");
        term_print(commands[i].description);
        term_putc('\n');
    }
}

/* ============================================================================
 *  SECTION 9: Line parser + dispatcher
 * ==========================================================================*/

static void terminal_process_line(const char *line) {
    if (line == NULL || line[0] == '\0') return;

    history_add(line);

    /* tokenize into argv on spaces (simple, no quoting support) */
    static char buf[LINE_MAX];
    k_strcpy(buf, line, LINE_MAX);

    char *argv[MAX_ARGS];
    int argc = 0;

    char *p = buf;
    while (*p && argc < MAX_ARGS) {
        while (*p == ' ') p++;      /* skip spaces */
            if (!*p) break;
            argv[argc++] = p;
        while (*p && *p != ' ') p++;
        if (*p) { *p = '\0'; p++; }
    }

    if (argc == 0) return;

    for (size_t i = 0; i < COMMAND_COUNT; i++) {
        if (k_strcmp(argv[0], commands[i].name) == 0) {
            commands[i].handler(argc, argv);
            return;
        }
    }

    term_print(argv[0]);
    term_print(": command not found (try 'help')\n");
}

/* ============================================================================
 *  SECTION 10: Init — call once from your kernel before processing any input
 * ==========================================================================*/

void terminal_init(void) {
    k_memset(fs_table, 0, sizeof(fs_table));
    history_count = 0;
    history_next = 0;
    current_dir[0] = '\0';
    term_clear();
    term_print("NovaShell ready. Type 'help' for a list of commands.\n");
}

/* Public: kernel_main calls this to draw the "A://..." prompt so it always
 * reflects wherever `goto` last left current_dir. */
void terminal_print_prompt(void) {
    print_current_path();
    term_print(" > ");
}

/* ============================================================================
 *  EXAMPLE INTEGRATION (remove this in your real kernel, or leave it — it's
 *  guarded and won't be compiled in unless you define TERMINAL_DEMO_MAIN)
 * ==========================================================================*/
#ifdef TERMINAL_DEMO_MAIN
void kernel_main(void) {
    terminal_init();
    terminal_process_line("say hello from my custom OS");
    terminal_process_line("mkfile notes.txt this is a test file");
    terminal_process_line("dir");
    terminal_process_line("show notes.txt");
    terminal_process_line("help");
    for (;;) { __asm__ volatile ("hlt"); }
}
#endif
