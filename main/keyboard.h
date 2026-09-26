#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define KEY_NONE 0
#define KEY_UP 256
#define KEY_DOWN 257
#define KEY_LEFT 258
#define KEY_RIGHT 259
#define KEY_ENTER 260
#define KEY_ESCAPE 261
#define KEY_BACKSPACE 262

void keyboard_begin(void);
int keyboard_read_key(void);
int keyboard_wait_key(void);
void keyboard_set_text_input(bool enabled);

#ifdef __cplusplus
}
#endif
