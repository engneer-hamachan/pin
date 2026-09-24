#include "breadboard.h"

#include "keyboard.h"
#include "widget.h"

#include <stdio.h>

int
decode_key(int key) {
  switch (key) {
  case 'k':
  case ';':
    return KEY_UP;
  case 'j':
  case '.':
    return KEY_DOWN;
  case 'h':
  case ',':
    return KEY_LEFT;
  case 'l':
  case '/':
    return KEY_RIGHT;
  default:
    return key;
  }
}

bool
move_cursor_by_key(Breadboard *board, int key) {
  switch (key) {
  case KEY_UP:
    move_cursor(board, 0, -1);
    return true;
  case KEY_DOWN:
    move_cursor(board, 0, 1);
    return true;
  case KEY_LEFT:
    move_cursor(board, -1, 0);
    return true;
  case KEY_RIGHT:
    move_cursor(board, 1, 0);
    return true;
  default:
    return false;
  }
}

// The browser build has no SD card, so it offers no save / load.
#if !defined(PIN_BOARD_WASM)
static void
save_file(Breadboard *board) {
  int slot_index = widget_run_menu("SAVE", SAVE_FILE_NAMES, SAVE_SLOT_COUNT, 0);

  if (slot_index < 0)
    return;

  const char *result = save_board(board, slot_index) ? "saved" : "save failed";

  snprintf(board->message, sizeof(board->message), "%s", result);
}

static void
load_file(Breadboard *board) {
  int slot_index = widget_run_menu("LOAD", SAVE_FILE_NAMES, SAVE_SLOT_COUNT, 0);

  if (slot_index < 0)
    return;

  if (!widget_run_confirm("Load board?"))
    return;

  const char *result = load_board(board, slot_index) ? "loaded" : "load failed";

  snprintf(board->message, sizeof(board->message), "%s", result);
}
#endif

static void
handle_edit_key(Breadboard *board, int key) {
  Part *part = find_part_at(board, find_cursor_hole_index(board));

  switch (key) {
  case KEY_ENTER:
  case 'a':
    start_placement(board);
    break;
  case ' ':
    if (part)
      operate_part(board, part);
    break;
  case '+':
  case '=':
    if (part)
      adjust_part(board, part, 1);
    break;
  case '-':
    if (part)
      adjust_part(board, part, -1);
    break;
  case 'x':
  case KEY_BACKSPACE:
    if (part)
      remove_part(board, part);
    break;
  case 'c':
    if (widget_run_confirm("Clear board?"))
      remove_all_parts(board);
    break;
#if !defined(PIN_BOARD_WASM)
  case 's':
    save_file(board);
    break;
  case 'o':
    load_file(board);
    break;
#endif
  case '?':
    board->help_visible = true;
    break;
  case 'q':
    if (widget_run_confirm("Quit?"))
      board->quit = true;
    break;
  default:
    break;
  }
}

void
handle_key(Breadboard *board, int key) {
  board->needs_redraw = true;
  board->message[0] = 0;

  if (board->help_visible) {
    board->help_visible = false;
    return;
  }

  key = decode_key(key);

  if (move_cursor_by_key(board, key))
    return;

  if (board->placing_entry)
    handle_placement_key(board, key);
  else
    handle_edit_key(board, key);
}
