#ifndef EDITOR_CANVAS_H
#define EDITOR_CANVAS_H

#include <stdint.h>

#include "canvas.h"
#include "core/render/screen.h"

#define EDIT_CHAR_WIDTH 6
#define EDIT_ROW_HEIGHT CANVAS_ROW_HEIGHT

typedef struct {
  int char_width;
  int row_height;
} EditorCanvas;

void clear_editor_canvas_row(void *context);

void draw_editor_canvas_row_text(
  void *context,
  int column,
  const char *text,
  int byte_length,
  uint32_t foreground,
  uint32_t background,
  int inverse
);

void push_editor_canvas_row(void *context, int row_index);

void fill_editor_canvas_row_span(
  void *context,
  int column,
  int column_count,
  uint32_t color
);

void draw_editor_canvas_row_underline(
  void *context,
  int column,
  int column_count,
  uint32_t color
);

void set_editor_canvas_font_size(void *context, int font_size);

void draw_editor_canvas_cursor(
  void *context,
  int column,
  int row_index,
  int visible,
  VimMode mode
);

void highlight_visible_row_text(
  VimCanvas *canvas,
  VimSyntax syntax,
  int column,
  const char *text,
  int text_byte_length,
  int visible_byte_begin,
  int visible_byte_end
);

#endif
