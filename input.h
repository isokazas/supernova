#ifndef INPUT_H
#define INPUT_H
#include <stdint.h>
typedef enum { INPUT_NONE, INPUT_KEY, INPUT_MOUSE } input_type_t;
typedef enum { KEY_NONE, KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_TAB, KEY_ENTER, KEY_CLOSE, KEY_BACKSPACE } key_code_t;
typedef struct {
    input_type_t type;
    key_code_t key;
    char character;
    int mouse_dx, mouse_dy;
    uint8_t mouse_buttons;
} input_event_t;
void input_init(void);
int input_poll(input_event_t *event);
#endif
