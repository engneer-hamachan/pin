#include "editor.h"

#include "canvas.h"
#include "keyboard.h"
#include "core/editor.h"
#include "core/text/utf8.h"
#include "port/editor_canvas.h"
#include "port/editor_file.h"
#include "port/editor_host.h"
#include "theme.h"

#include <stdlib.h>
#include <string.h>

#define ASCII_ESCAPE 27
#define UTF8_MAXIMUM_BYTE_LENGTH 4

static VimStatus
handle_editor_byte(Vim *core, int first_byte) {
  if (first_byte == ASCII_ESCAPE) {
    char sequence[2];
    int sequence_byte_length = read_escape_sequence(sequence);

    return vim_handle_esc(core, sequence, sequence_byte_length);
  }

  char character[UTF8_MAXIMUM_BYTE_LENGTH];
  int character_byte_length = vim_utf8_byte_length((uint8_t)first_byte);

  if (character_byte_length < 1)
    character_byte_length = 1;

  if (character_byte_length > UTF8_MAXIMUM_BYTE_LENGTH)
    character_byte_length = UTF8_MAXIMUM_BYTE_LENGTH;

  character[0] = (char)first_byte;

  for (int i = 1; i < character_byte_length; i++)
    character[i] = (char)editor_wait_byte();

  return vim_handle_key(core, first_byte, character, character_byte_length);
}

static void
run_editor_loop(Vim *core, VimCanvas *canvas) {
  core->screen.redraw_mode = VIM_REDRAW_ALL;
  vim_screen_refresh_if_needed(&core->screen, canvas);

  for (;;) {
    VimStatus status = handle_editor_byte(core, editor_wait_byte());

    if (core->screen.buffer.dirty >= VIM_DIRTY_CONTENT)
      vim_clear_diagnostics(core);

    vim_screen_refresh_if_needed(&core->screen, canvas);

    if (status == VIM_QUIT)
      return;

    if (status == VIM_SAVE || status == VIM_SAVE_QUIT) {
      int saved = save_edit_file(core);

      vim_handle_after_save(core, saved);
      core->screen.redraw_mode = VIM_REDRAW_ALL;
      vim_screen_refresh_if_needed(&core->screen, canvas);

      if (status == VIM_SAVE_QUIT && saved)
        return;
    }
  }
}

bool
run_editor(const char *path) {
  Vim *core = (Vim *)malloc(sizeof(Vim));

  if (core == NULL)
    return false;

  int columns;
  int rows;

  compute_editor_grid(&columns, &rows);
  vim_init(core, columns, rows);
  core->screen.highlight = highlight_visible_row_text;

  int path_byte_length = (int)strlen(path);

  vim_set_filepath(core, path, path_byte_length);

  VimString content;

  vim_string_init(&content);

  if (load_edit_file(path, path_byte_length, &content))
    vim_load_text(core, content.bytes, content.byte_length);

  vim_string_free(&content);

  EditorCanvas editor_canvas = {EDIT_CHAR_WIDTH, EDIT_ROW_HEIGHT};
  VimCanvas canvas = {
    .context = &editor_canvas,
    .clear_row = clear_editor_canvas_row,
    .draw_row_text = draw_editor_canvas_row_text,
    .push_row = push_editor_canvas_row,
    .fill_row_span = fill_editor_canvas_row_span,
    .draw_row_underline = draw_editor_canvas_row_underline,
    .set_font_size = set_editor_canvas_font_size,
    .draw_cursor = draw_editor_canvas_cursor,
  };

  canvas_screen_fill(THEME_BACKGROUND_COLOR);
  core->active_canvas = &canvas;
  keyboard_set_text_input(true);
  run_editor_loop(core, &canvas);
  keyboard_set_text_input(false);
  core->active_canvas = NULL;

  vim_free(core);
  free(core);
  return true;
}
