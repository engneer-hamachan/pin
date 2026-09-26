#include "keyboard.h"

#include <emscripten.h>
#include <emscripten/html5.h>
#include <stdbool.h>
#include <string.h>

#define KEY_QUEUE_CAPACITY 16
#define WAIT_POLL_INTERVAL_MS 10

static int key_queue[KEY_QUEUE_CAPACITY];
static int key_queue_head = 0;
static int key_queue_count = 0;

static void
push_key(int key) {
  if (key_queue_count == KEY_QUEUE_CAPACITY)
    return;

  key_queue[(key_queue_head + key_queue_count) % KEY_QUEUE_CAPACITY] = key;
  key_queue_count++;
}

static int
pop_key(void) {
  if (key_queue_count == 0)
    return KEY_NONE;

  int key = key_queue[key_queue_head];

  key_queue_head = (key_queue_head + 1) % KEY_QUEUE_CAPACITY;
  key_queue_count--;

  return key;
}

static int
translate_key(const char *name) {
  if (strcmp(name, "ArrowUp") == 0)
    return KEY_UP;
  if (strcmp(name, "ArrowDown") == 0)
    return KEY_DOWN;
  if (strcmp(name, "ArrowLeft") == 0)
    return KEY_LEFT;
  if (strcmp(name, "ArrowRight") == 0)
    return KEY_RIGHT;
  if (strcmp(name, "Enter") == 0)
    return KEY_ENTER;
  if (strcmp(name, "Escape") == 0 || strcmp(name, "`") == 0)
    return KEY_ESCAPE;
  if (strcmp(name, "Backspace") == 0)
    return KEY_BACKSPACE;
  if (strcmp(name, "Tab") == 0)
    return '\t';

  if (name[0] >= 0x20 && name[0] <= 0x7E && name[1] == '\0')
    return name[0];

  return KEY_NONE;
}

static bool
handle_key_down(
  int event_type,
  const EmscriptenKeyboardEvent *event,
  void *user_data
) {
  (void)event_type;
  (void)user_data;

  if (event->metaKey || event->altKey)
    return false;

  if (event->ctrlKey) {
    if (event->key[0] < 'a' || event->key[0] > 'z' || event->key[1] != '\0')
      return false;

    push_key(event->key[0] & 0x1F);
    return true;
  }

  int key = translate_key(event->key);

  if (key == KEY_NONE)
    return false;

  push_key(key);
  return true;
}

EM_JS(void, set_text_input_class, (int enabled), {
  document.body.classList.toggle("text-input", enabled !== 0);

  if (!enabled)
    document.getElementById("text-input").blur();
});

void
keyboard_set_text_input(bool enabled) {
  set_text_input_class(enabled);
}

void
keyboard_begin(void) {
  emscripten_set_keydown_callback(
    EMSCRIPTEN_EVENT_TARGET_DOCUMENT,
    NULL,
    true,
    handle_key_down
  );
}

int
keyboard_read_key(void) {
  return pop_key();
}

int
keyboard_wait_key(void) {
  for (;;) {
    int key = keyboard_read_key();

    if (key != KEY_NONE)
      return key;

    emscripten_sleep(WAIT_POLL_INTERVAL_MS);
  }
}
