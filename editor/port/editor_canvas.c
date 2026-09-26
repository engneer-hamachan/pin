#include "port/editor_canvas.h"

#include "port/editor_host.h"
#include "theme.h"

#include "core/syntax/picoruby/highlight.h"
#include "core/text/utf8.h"

#include <stdint.h>
#include <string.h>

#define CURSOR_THICKNESS 2

void
clear_editor_canvas_row(void *context) {
  (void)context;
  canvas_row_fill(THEME_BACKGROUND_COLOR);
}

void
draw_editor_canvas_row_text(
  void *context,
  int column,
  const char *text,
  int byte_length,
  uint32_t foreground,
  uint32_t background,
  int inverse
) {

  EditorCanvas *canvas = (EditorCanvas *)context;

  if (byte_length <= 0)
    return;

  if (inverse) {
    uint32_t previous_foreground = foreground;
    foreground = background;
    background = previous_foreground;
  }

  int pixel_left = column * canvas->char_width;

  canvas_row_fill_rect(
    pixel_left,
    0,
    vim_display_width(text, byte_length) * canvas->char_width,
    canvas->row_height,
    background
  );

  char text_buffer[256];

  int copy_byte_length = (int)sizeof(text_buffer) - 1;

  if (byte_length < copy_byte_length)
    copy_byte_length = byte_length;

  memcpy(text_buffer, text, copy_byte_length);

  text_buffer[copy_byte_length] = '\0';

  canvas_row_text(pixel_left, 0, text_buffer, foreground);
}

static int
compute_row_pixel_top(EditorCanvas *canvas, int row_index) {
  int last_row_index = CANVAS_HEIGHT / canvas->row_height - 1;

  if (row_index == last_row_index)
    return CANVAS_HEIGHT - canvas->row_height;

  return row_index * canvas->row_height;
}

void
push_editor_canvas_row(void *context, int row_index) {
  EditorCanvas *canvas = (EditorCanvas *)context;

  canvas_row_commit(compute_row_pixel_top(canvas, row_index));
  mark_editor_canvas_changed();
}

void
fill_editor_canvas_row_span(
  void *context,
  int column,
  int column_count,
  uint32_t color
) {
  EditorCanvas *canvas = (EditorCanvas *)context;

  if (column_count <= 0)
    return;

  canvas_row_fill_rect(
    column * canvas->char_width,
    0,
    column_count * canvas->char_width,
    canvas->row_height,
    color
  );
}

void
draw_editor_canvas_row_underline(
  void *context,
  int column,
  int column_count,
  uint32_t color
) {
  EditorCanvas *canvas = (EditorCanvas *)context;

  if (column_count <= 0)
    return;

  int pixel_left = column * canvas->char_width;
  int pixel_right = (column + column_count) * canvas->char_width - 1;
  int pixel_top = canvas->row_height - 1;

  canvas_row_line(pixel_left, pixel_top, pixel_right, pixel_top, color);
}

void
set_editor_canvas_font_size(void *context, int font_size) {
  (void)context;
  (void)font_size;
}

void
draw_editor_canvas_cursor(
  void *context,
  int column,
  int row_index,
  int visible,
  VimMode mode
) {
  EditorCanvas *canvas = (EditorCanvas *)context;

  if (!visible)
    return;

  int pixel_left = column * canvas->char_width;
  int pixel_top = compute_row_pixel_top(canvas, row_index);

  if (mode == VIM_MODE_INSERT)
    canvas_fill_rect(
      pixel_left,
      pixel_top + 1,
      CURSOR_THICKNESS,
      canvas->row_height - 2,
      THEME_SELECTED_COLOR
    );
  else
    canvas_fill_rect(
      pixel_left,
      pixel_top + canvas->row_height - CURSOR_THICKNESS,
      canvas->char_width,
      CURSOR_THICKNESS,
      THEME_SELECTED_COLOR
    );

  mark_editor_canvas_changed();
}

typedef struct {
  VimCanvas *canvas;
  int column;
  int next_segment_byte_offset;
  int visible_byte_begin;
  int visible_byte_end;
} highlight_canvas_context;

static void
draw_highlight_segment(
  void *writer_context,
  const char *text,
  int byte_length,
  uint32_t color
) {

  highlight_canvas_context *context =
    (highlight_canvas_context *)writer_context;

  int segment_byte_begin = context->next_segment_byte_offset;
  int segment_byte_end = segment_byte_begin + byte_length;

  context->next_segment_byte_offset = segment_byte_end;

  int drawn_byte_begin = segment_byte_begin;
  if (drawn_byte_begin < context->visible_byte_begin)
    drawn_byte_begin = context->visible_byte_begin;

  int drawn_byte_end = segment_byte_end;
  if (drawn_byte_end > context->visible_byte_end)
    drawn_byte_end = context->visible_byte_end;

  if (drawn_byte_end <= drawn_byte_begin)
    return;

  const char *drawn_text = text + (drawn_byte_begin - segment_byte_begin);
  int drawn_byte_length = drawn_byte_end - drawn_byte_begin;

  context->canvas->draw_row_text(
    context->canvas->context,
    context->column,
    drawn_text,
    drawn_byte_length,
    color,
    THEME_BACKGROUND_COLOR,
    0
  );

  context->column += vim_display_width(drawn_text, drawn_byte_length);
}

static void
run_ruby_highlight(
  highlight_canvas_context *writer_context,
  const char *text,
  int text_byte_length
) {

  editor_highlight_context_t highlight_context;

  editor_highlight_init(
    &highlight_context,
    (const uint8_t *)text,
    text_byte_length,
    draw_highlight_segment,
    writer_context
  );

  editor_highlight_run(&highlight_context);
}

void
highlight_visible_row_text(
  VimCanvas *canvas,
  VimSyntax syntax,
  int column,
  const char *text,
  int text_byte_length,
  int visible_byte_begin,
  int visible_byte_end
) {

  if (visible_byte_end <= visible_byte_begin)
    return;

  highlight_canvas_context writer_context = {
    canvas,
    column,
    0,
    visible_byte_begin,
    visible_byte_end,
  };

  switch (syntax) {
  case VIM_SYNTAX_RUBY:
    run_ruby_highlight(&writer_context, text, text_byte_length);
    break;

  default:
    canvas->draw_row_text(
      canvas->context,
      column,
      text + visible_byte_begin,
      visible_byte_end - visible_byte_begin,
      THEME_TEXT_COLOR,
      THEME_BACKGROUND_COLOR,
      0
    );
    break;
  }
}
