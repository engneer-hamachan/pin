#include "breadboard.h"

#define PINO_LOWER_ROW_OFFSET 5

bool
has_footprint(PartKind kind) {
  switch (kind) {
  case PART_KIND_RGB_LED:
  case PART_KIND_SEVEN_SEGMENT:
  case PART_KIND_TACT_SWITCH:
  case PART_KIND_SLIDE_SWITCH:
  case PART_KIND_NPN:
  case PART_KIND_RELAY:
  case PART_KIND_VOLUME:
  case PART_KIND_PINO:
    return true;
  default:
    return false;
  }
}

int
count_terminals(PartKind kind) {
  switch (kind) {
  case PART_KIND_RGB_LED:
  case PART_KIND_TACT_SWITCH:
    return 4;
  case PART_KIND_SEVEN_SEGMENT:
    return 10;
  case PART_KIND_SLIDE_SWITCH:
  case PART_KIND_NPN:
  case PART_KIND_VOLUME:
    return 3;
  case PART_KIND_RELAY:
    return 5;
  case PART_KIND_PINO:
    return PINO_PIN_COUNT;
  default:
    return 2;
  }
}

int
list_contact_terminal_pairs(
  PartKind kind,
  int contact_pairs[CONTACT_PAIR_CAPACITY][2]
) {

  switch (kind) {
  case PART_KIND_WIRE:
    contact_pairs[0][0] = 0;
    contact_pairs[0][1] = 1;
    return 1;
  case PART_KIND_SEVEN_SEGMENT:
    contact_pairs[0][0] = 2;
    contact_pairs[0][1] = 7;
    return 1;
  case PART_KIND_TACT_SWITCH:
    contact_pairs[0][0] = 0;
    contact_pairs[0][1] = 2;
    contact_pairs[1][0] = 1;
    contact_pairs[1][1] = 3;
    return 2;
  default:
    return 0;
  }
}

static int
compute_footprint_row_offset(PartKind kind, int terminal_index) {
  switch (kind) {
  case PART_KIND_PINO:
    return terminal_index < PINO_PIN_COUNT / 2 ? PINO_LOWER_ROW_OFFSET : 0;
  case PART_KIND_SEVEN_SEGMENT:
    return terminal_index < 5 ? 0 : 3;
  case PART_KIND_TACT_SWITCH:
    return terminal_index / 2;
  case PART_KIND_RELAY:
    return terminal_index < 2 ? 0 : 3;
  default:
    return 0;
  }
}

static int
compute_footprint_column_offset(PartKind kind, int terminal_index) {
  switch (kind) {
  case PART_KIND_PINO:
    return terminal_index < PINO_PIN_COUNT / 2
             ? terminal_index
             : PINO_PIN_COUNT - 1 - terminal_index;
  case PART_KIND_SEVEN_SEGMENT:
    return terminal_index % 5;
  case PART_KIND_TACT_SWITCH:
    return terminal_index % 2 * 2;
  case PART_KIND_RELAY:
    return terminal_index < 2 ? terminal_index * 2 : terminal_index - 2;
  default:
    return terminal_index;
  }
}

bool
compute_footprint_hole_indices(
  PartKind kind,
  int anchor_row,
  int anchor_column,
  int *hole_indices
) {

  for (int i = 0; i < count_terminals(kind); i++) {
    int row = anchor_row + compute_footprint_row_offset(kind, i);
    int column = anchor_column + compute_footprint_column_offset(kind, i);

    if (!hole_exists(row, column))
      return false;

    hole_indices[i] = build_hole_index(row, column);
  }

  return true;
}
