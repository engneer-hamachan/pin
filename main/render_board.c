#include "breadboard.h"

#include "canvas.h"
#include "theme.h"
#include "widget.h"

#define BOARD_X 12
#define BOARD_WIDTH 216
#define BOARD_COLOR 0xE8E2D0
#define GROOVE_COLOR 0xC4BCAA
#define POSITIVE_RAIL_COLOR 0xD03030
#define NEGATIVE_RAIL_COLOR 0x3050D0
#define HOLE_COLOR 0x4A4A4A
#define CURSOR_COLOR 0x00C0FF
#define PLACEMENT_ALLOWED_COLOR 0x20C020
#define PLACEMENT_BLOCKED_COLOR 0xFF0000
#define CONSOLE_LINE_HEIGHT 12

static void
draw_rail_line(int y, uint32_t color) {
  canvas_line(BOARD_X + 2, y, BOARD_X + BOARD_WIDTH - 3, y, color);
}

static void
draw_holes(void) {
  for (int row = 0; row < BOARD_ROW_COUNT; row++) {
    int y = compute_row_y(row);

    for (int column = 0; column < BOARD_COLUMN_COUNT; column++)
      canvas_fill_rect(compute_column_x(column) - 1, y - 1, 3, 3, HOLE_COLOR);
  }
}

void
draw_board(void) {
  int top_y = compute_row_y(0) - 3 - 1;
  int bottom_y = compute_row_y(BOARD_ROW_COUNT - 1) + 3 + 2;
  int groove_y = (compute_row_y(6) + compute_row_y(7)) / 2;

  canvas_fill_rect(BOARD_X, top_y, BOARD_WIDTH, bottom_y - top_y, BOARD_COLOR);
  canvas_fill_rect(BOARD_X, groove_y - 1, BOARD_WIDTH, 3, GROOVE_COLOR);
  draw_rail_line(compute_row_y(0) - 3, POSITIVE_RAIL_COLOR);
  draw_rail_line(compute_row_y(1) + 3, NEGATIVE_RAIL_COLOR);
  draw_rail_line(compute_row_y(12) - 3, POSITIVE_RAIL_COLOR);
  draw_rail_line(compute_row_y(BOARD_ROW_COUNT - 1) + 3, NEGATIVE_RAIL_COLOR);
  draw_holes();
}

static void
draw_hole_marker(int hole_index, uint32_t color) {
  canvas_rect(
    compute_hole_x(hole_index) - 3,
    compute_hole_y(hole_index) - 3,
    7,
    7,
    color
  );
}

void
draw_placement_preview(Breadboard *board) {
  int hole_indices[PART_TERMINAL_CAPACITY];
  int hole_count = compute_placement_hole_indices(board, hole_indices);

  if (hole_count == 0)
    return;

  uint32_t color = is_placement_allowed(board) ? PLACEMENT_ALLOWED_COLOR
                                               : PLACEMENT_BLOCKED_COLOR;

  if (hole_count == 2 && !has_footprint(board->placing_entry->kind)) {
    canvas_line(
      compute_hole_x(hole_indices[0]),
      compute_hole_y(hole_indices[0]),
      compute_hole_x(hole_indices[1]),
      compute_hole_y(hole_indices[1]),
      color
    );
  }

  for (int i = 0; i < hole_count; i++)
    draw_hole_marker(hole_indices[i], color);
}

void
draw_cursor(const Breadboard *board) {
  draw_hole_marker(find_cursor_hole_index(board), CURSOR_COLOR);
}

static void
draw_console(void) {
  int top_y = WIDGET_HEADER_HEIGHT + 2;
  int line_count = count_pino_console_lines();

  widget_draw_panel(
    8,
    top_y,
    CANVAS_WIDTH - 16,
    PINO_CONSOLE_LINE_COUNT * CONSOLE_LINE_HEIGHT + 8
  );

  for (int i = 0; i < line_count; i++)
    canvas_text(
      14,
      top_y + 4 + i * CONSOLE_LINE_HEIGHT,
      read_pino_console_line(i),
      THEME_TEXT_COLOR
    );
}

void
draw_screen(Breadboard *board) {
  char title[TEXT_SIZE];
  char right_text[TEXT_SIZE];

  format_header_title(board, title, sizeof title);
  format_header_right(board, right_text, sizeof right_text);

  while (canvas_begin_band()) {
    canvas_fill(THEME_BACKGROUND_COLOR);
    draw_board();
    draw_parts(board);

    if (board->placing_entry)
      draw_placement_preview(board);

    draw_cursor(board);
    widget_draw_header(title, right_text);

    if (board->console_visible)
      draw_console();

    canvas_push_band();
  }
}

static int
divide_rounding_down(int dividend, int divisor) {
  int quotient = dividend / divisor;

  if (dividend % divisor != 0 && (dividend < 0) != (divisor < 0))
    quotient--;

  return quotient;
}

static uint32_t
blend_channel(uint32_t dark_value, uint32_t bright_value, int level) {
  int dark_channel = (int)(dark_value & 0xFF);
  int bright_channel = (int)(bright_value & 0xFF);
  int step = divide_rounding_down(
    (bright_channel - dark_channel) * level,
    LEVEL_MAXIMUM
  );

  return (uint32_t)(dark_channel + step);
}

uint32_t
blend_color(uint32_t dark_color, uint32_t bright_color, int level) {
  uint32_t red = blend_channel(dark_color >> 16, bright_color >> 16, level);
  uint32_t green = blend_channel(dark_color >> 8, bright_color >> 8, level);
  uint32_t blue = blend_channel(dark_color, bright_color, level);

  return (red << 16) | (green << 8) | blue;
}
