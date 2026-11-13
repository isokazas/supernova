#include "keyboard.h"

/* Include the existing terminal implementation here so the small input loop
 * can call its private prompt, output, and command-dispatch helpers. */
#include "terminal.c"

void terminal_run(void) {
    char line[LINE_MAX];

    terminal_print_prompt();
    for (;;) {
        size_t length = 0;

        for (;;) {
            char c = wait_and_get_key();

            if (c == '\n') {
                term_putc('\n');
                line[length] = '\0';
                terminal_process_line(line);
                terminal_print_prompt();
                break;
            }

            if (c == '\b') {
                if (length > 0) {
                    length--;
                    term_putc('\b');
                }
                continue;
            }

            if (c >= 32 && c <= 126 && length < LINE_MAX - 1) {
                line[length++] = c;
                term_putc(c);
            }
        }
    }
}
