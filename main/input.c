#include "breadboard.h"

#include "editor.h"
#include "keyboard.h"
#include "page.h"
#include "widget.h"

#include <stdio.h>

static const PageLine HELP_LINES[] = {
  {PAGE_LINE_HEADING, NULL, "KEYS"},
  {PAGE_LINE_KEY, "hjkl", "move the cursor"},
  {PAGE_LINE_KEY, "arrows", "also move the cursor"},
  {PAGE_LINE_KEY, "a/Enter", "add a part"},
  {PAGE_LINE_KEY, "space", "press / flip a switch"},
  {PAGE_LINE_KEY, "+ -", "change a value"},
  {PAGE_LINE_KEY, "i", "part info"},
  {PAGE_LINE_KEY, "x / BS", "remove a part"},
  {PAGE_LINE_KEY, "c", "clear the board"},
  {PAGE_LINE_KEY, "e", "edit Pin_data/app.rb"},
  {PAGE_LINE_KEY, "v", "show the pino output"},
#if !defined(PIN_BOARD_WASM)
  {PAGE_LINE_KEY, "s", "save the board"},
  {PAGE_LINE_KEY, "o", "load the board"},
#endif
  {PAGE_LINE_KEY, "?", "this help"},
  {PAGE_LINE_KEY, "q", "quit"},
  {PAGE_LINE_BLANK, NULL, NULL},
  {PAGE_LINE_HEADING, NULL, "WHILE PLACING"},
  {PAGE_LINE_KEY, "Enter", "set the next pin"},
  {PAGE_LINE_KEY, "space", "also sets the next pin"},
  {PAGE_LINE_KEY, "` (ESC)", "cancel"},
  {PAGE_LINE_KEY, "q", "also cancels"},
};

#define HELP_LINE_COUNT COUNT_PAGE_LINES(HELP_LINES)

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
edit_pino_program(Breadboard *board) {
  stop_pino_program();

  const char *result = run_editor(PINO_PROGRAM_PATH) ? "" : "edit failed";

  snprintf(board->message, sizeof(board->message), "%s", result);
  restart_pino_program();
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
  case 'i':
    if (part)
      show_part_info(part->kind);
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
  case 'e':
    edit_pino_program(board);
    break;
  case 'v':
    board->console_visible = true;
    break;
  case '?':
    show_page("HELP", HELP_LINES, HELP_LINE_COUNT);
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

  if (board->console_visible) {
    board->console_visible = false;
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
