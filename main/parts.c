#include "breadboard.h"

#include <string.h>

#define TACT_SWITCH_PRESS_FRAMES 6
#define DEFAULT_RESISTANCE_INDEX 2
#define DEFAULT_KNOB_POSITION 5
#define DEFAULT_LIGHT_LEVEL 5

Part *
find_part_at(Breadboard *board, int hole_index) {
  for (int i = 0; i < board->part_count; i++) {
    Part *part = &board->parts[i];

    for (int j = 0; j < part->terminal_count; j++) {
      if (part->terminal_hole_indices[j] == hole_index)
        return part;
    }
  }

  return NULL;
}

bool
are_holes_free(Breadboard *board, const int *hole_indices, int hole_count) {
  for (int i = 0; i < hole_count; i++) {
    if (find_part_at(board, hole_indices[i]))
      return false;

    for (int j = i + 1; j < hole_count; j++) {
      if (hole_indices[j] == hole_indices[i])
        return false;
    }
  }

  return true;
}

int
count_parts_of_kind(const Breadboard *board, PartKind kind) {
  int count = 0;

  for (int i = 0; i < board->part_count; i++) {
    if (board->parts[i].kind == kind)
      count++;
  }

  return count;
}

static void
init_part(
  Part *part,
  PartKind kind,
  const int *terminal_hole_indices,
  int color_index
) {

  memset(part, 0, sizeof(*part));
  part->kind = kind;
  part->terminal_count = count_terminals(kind);
  part->color_index = color_index;

  for (int i = 0; i < part->terminal_count; i++) {
    part->terminal_hole_indices[i] = terminal_hole_indices[i];
    part->terminal_node_indices[i] = find_base_net(terminal_hole_indices[i]);
  }

  switch (kind) {
  case PART_KIND_RESISTOR:
    part->resistance_index = DEFAULT_RESISTANCE_INDEX;
    break;
  case PART_KIND_VOLUME:
    part->knob_position = DEFAULT_KNOB_POSITION;
    break;
  case PART_KIND_CDS:
    part->light_level = DEFAULT_LIGHT_LEVEL;
    break;
  default:
    break;
  }
}

void
add_part(
  Breadboard *board,
  PartKind kind,
  const int *terminal_hole_indices,
  int color_index
) {

  if (board->part_count == PART_CAPACITY)
    return;

  if (kind == PART_KIND_WIRE)
    color_index = count_parts_of_kind(board, PART_KIND_WIRE) % WIRE_COLOR_COUNT;

  init_part(
    &board->parts[board->part_count],
    kind,
    terminal_hole_indices,
    color_index
  );
  board->part_count++;
  update_circuit_structure(board);
}

void
remove_part(Breadboard *board, Part *part) {
  int part_index = (int)(part - board->parts);

  memmove(
    part,
    part + 1,
    (size_t)(board->part_count - part_index - 1) * sizeof(Part)
  );
  board->part_count--;
  update_circuit_structure(board);
}

void
remove_all_parts(Breadboard *board) {
  board->part_count = 0;
  update_circuit_structure(board);
}

void
update_circuit_structure(Breadboard *board) {
  board->time_dependent = count_parts_of_kind(board, PART_KIND_CAPACITOR) > 0 ||
                          count_parts_of_kind(board, PART_KIND_RELAY) > 0;
  board->short_circuit = false;
  rebuild_circuit(board);
  board->circuit_dirty = true;
}

void
operate_part(Breadboard *board, Part *part) {
  switch (part->kind) {
  case PART_KIND_TACT_SWITCH:
    part->pressed_frames = TACT_SWITCH_PRESS_FRAMES;
    break;
  case PART_KIND_SLIDE_SWITCH:
    part->slide_position = 1 - part->slide_position;
    break;
  default:
    return;
  }

  board->circuit_dirty = true;
}

void
adjust_part(Breadboard *board, Part *part, int step) {
  switch (part->kind) {
  case PART_KIND_RESISTOR:
    part->resistance_index =
      clamp_integer(part->resistance_index + step, 0, RESISTOR_VALUE_COUNT - 1);
    break;
  case PART_KIND_VOLUME:
    part->knob_position =
      clamp_integer(part->knob_position + step, 0, KNOB_POSITION_MAXIMUM);
    break;
  case PART_KIND_CDS:
    part->light_level =
      clamp_integer(part->light_level + step, 0, LIGHT_LEVEL_MAXIMUM);
    break;
  default:
    return;
  }

  board->circuit_dirty = true;
}

int
clamp_integer(int value, int minimum, int maximum) {
  if (value < minimum)
    return minimum;

  if (value > maximum)
    return maximum;

  return value;
}
