#include "breadboard.h"

#include "keyboard.h"
#include "widget.h"

static int
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

static bool
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
