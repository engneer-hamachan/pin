#include "port/editor_host.h"

#include "canvas.h"
#include "keyboard.h"
#include "port/editor_canvas.h"

#include <stdbool.h>

#define BYTE_QUEUE_CAPACITY 4
#define ASCII_ESCAPE 27
#define ASCII_CARRIAGE_RETURN 13
#define ASCII_DELETE 127

static int byte_queue[BYTE_QUEUE_CAPACITY];
static int byte_queue_head = 0;
static int byte_queue_count = 0;
static bool canvas_changed = false;

void
compute_editor_grid(int *columns, int *rows) {
  *columns = CANVAS_WIDTH / EDIT_CHAR_WIDTH;
  *rows = CANVAS_HEIGHT / EDIT_ROW_HEIGHT;
}

void
mark_editor_canvas_changed(void) {
  canvas_changed = true;
}

static void
push_byte(int byte) {
  if (byte_queue_count == BYTE_QUEUE_CAPACITY)
    return;

  byte_queue[(byte_queue_head + byte_queue_count) % BYTE_QUEUE_CAPACITY] =
    byte;
  byte_queue_count++;
}

static int
pop_byte(void) {
  int byte = byte_queue[byte_queue_head];

  byte_queue_head = (byte_queue_head + 1) % BYTE_QUEUE_CAPACITY;
  byte_queue_count--;
  return byte;
}

static void
push_arrow_sequence(char letter) {
  push_byte(ASCII_ESCAPE);
  push_byte('[');
  push_byte(letter);
}

static void
push_key_bytes(int key) {
  switch (key) {
  case KEY_UP:
    push_arrow_sequence('A');
    break;
  case KEY_DOWN:
    push_arrow_sequence('B');
    break;
  case KEY_RIGHT:
    push_arrow_sequence('C');
    break;
  case KEY_LEFT:
    push_arrow_sequence('D');
    break;
  case KEY_ENTER:
    push_byte(ASCII_CARRIAGE_RETURN);
    break;
  case KEY_ESCAPE:
    push_byte(ASCII_ESCAPE);
    break;
  case KEY_BACKSPACE:
    push_byte(ASCII_DELETE);
    break;
  default:
    push_byte(key);
    break;
  }
}

int
editor_wait_byte(void) {
  while (byte_queue_count == 0) {
    if (canvas_changed) {
      canvas_screen_push();
      canvas_changed = false;
    }

    push_key_bytes(keyboard_wait_key());
  }

  return pop_byte();
}

int
read_escape_sequence(char *sequence) {
  int sequence_byte_count = 0;

  while (sequence_byte_count < 2 && byte_queue_count > 0)
    sequence[sequence_byte_count++] = (char)pop_byte();

  return sequence_byte_count;
}
