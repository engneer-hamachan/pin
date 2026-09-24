#include "breadboard.h"

#include <stdio.h>

#define RAIL_ROW_COUNT 2
#define BOTTOM_RAIL_FIRST_ROW 12
#define LOWER_STRIP_FIRST_ROW 7
#define STRIP_ROW_NAMES "abcdefghij"

int
build_hole_index(int row, int column) {
  return row * BOARD_COLUMN_COUNT + column;
}

int
read_hole_row(int hole_index) {
  return hole_index / BOARD_COLUMN_COUNT;
}

int
read_hole_column(int hole_index) {
  return hole_index % BOARD_COLUMN_COUNT;
}

bool
hole_exists(int row, int column) {
  return row >= 0 && row < BOARD_ROW_COUNT && column >= 0 &&
         column < BOARD_COLUMN_COUNT;
}

int
compute_row_y(int row) {
  int y = 21 + row * 7;

  if (row >= RAIL_ROW_COUNT)
    y += 4;

  if (row >= LOWER_STRIP_FIRST_ROW)
    y += 7;

  if (row >= BOTTOM_RAIL_FIRST_ROW)
    y += 4;

  return y;
}

int
compute_column_x(int column) {
  return 18 + column * 7;
}

int
compute_hole_x(int hole_index) {
  return compute_column_x(read_hole_column(hole_index));
}

int
compute_hole_y(int hole_index) {
  return compute_row_y(read_hole_row(hole_index));
}

int
find_base_net(int hole_index) {
  int row = read_hole_row(hole_index);
  int column = read_hole_column(hole_index);

  if (row < RAIL_ROW_COUNT)
    return row;

  if (row >= BOTTOM_RAIL_FIRST_ROW)
    return 2 + row - BOTTOM_RAIL_FIRST_ROW;

  if (row < LOWER_STRIP_FIRST_ROW)
    return 4 + column;

  return 4 + BOARD_COLUMN_COUNT + column;
}

void
format_hole_name(int hole_index, char *text, size_t text_size) {
  int row = read_hole_row(hole_index);
  int column_number = read_hole_column(hole_index) + 1;

  if (row >= RAIL_ROW_COUNT && row < BOTTOM_RAIL_FIRST_ROW) {
    snprintf(
      text,
      text_size,
      "%c%d",
      STRIP_ROW_NAMES[row - RAIL_ROW_COUNT],
      column_number
    );
    return;
  }

  snprintf(
    text,
    text_size,
    "%s %d",
    find_rail_name(find_base_net(hole_index)),
    column_number
  );
}

int
find_cursor_hole_index(const Breadboard *board) {
  return build_hole_index(board->cursor_row, board->cursor_column);
}

void
move_cursor(Breadboard *board, int column_step, int row_step) {
  int row = board->cursor_row + row_step;
  int column = board->cursor_column + column_step;

  if (!hole_exists(row, column))
    return;

  board->cursor_row = row;
  board->cursor_column = column;
}
