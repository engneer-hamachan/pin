#include "breadboard.h"

#include "keyboard.h"
#include "widget.h"

#include <stdio.h>

#define CATEGORY_COUNT 5
#define CATEGORY_ENTRY_CAPACITY 6
#define NO_HOLE_INDEX -1

typedef struct {
  const char *label;
  int entry_count;
  PlacementEntry entries[CATEGORY_ENTRY_CAPACITY];
} PlacementCategory;

static const PlacementCategory CATEGORIES[CATEGORY_COUNT] = {
  {
    "Wire / passive",
    5,
    {
      {"Jumper wire", PART_KIND_WIRE, 0},
      {"Resistor", PART_KIND_RESISTOR, 0},
      {"Volume 10k", PART_KIND_VOLUME, 0},
      {"CdS", PART_KIND_CDS, 0},
      {"Capacitor 1000uF", PART_KIND_CAPACITOR, 0},
    },
  },
  {
    "LED / display",
    6,
    {
      {"LED red", PART_KIND_LED, 0},
      {"LED yellow", PART_KIND_LED, 1},
      {"LED green", PART_KIND_LED, 2},
      {"LED blue", PART_KIND_LED, 3},
      {"RGB LED", PART_KIND_RGB_LED, 0},
      {"7-segment LED", PART_KIND_SEVEN_SEGMENT, 0},
    },
  },
  {
    "Switch",
    3,
    {
      {"Tact switch", PART_KIND_TACT_SWITCH, 0},
      {"Slide switch", PART_KIND_SLIDE_SWITCH, 0},
      {"Relay", PART_KIND_RELAY, 0},
    },
  },
  {
    "Power / output",
    3,
    {
      {"Battery 5V", PART_KIND_BATTERY, 0},
      {"Buzzer", PART_KIND_BUZZER, 0},
      {"Motor", PART_KIND_MOTOR, 0},
    },
  },
  {
    "Semiconductor",
    2,
    {
      {"Diode", PART_KIND_DIODE, 0},
      {"NPN transistor", PART_KIND_NPN, 0},
    },
  },
};

int
count_placement_entries(void) {
  int entry_count = 0;

  for (int i = 0; i < CATEGORY_COUNT; i++)
    entry_count += CATEGORIES[i].entry_count;

  return entry_count;
}

const PlacementEntry *
find_placement_entry(int entry_index) {
  for (int i = 0; i < CATEGORY_COUNT; i++) {
    if (entry_index < CATEGORIES[i].entry_count)
      return &CATEGORIES[i].entries[entry_index];

    entry_index -= CATEGORIES[i].entry_count;
  }

  return NULL;
}

void
start_placement(Breadboard *board) {
  const char *category_labels[CATEGORY_COUNT];

  for (int i = 0; i < CATEGORY_COUNT; i++)
    category_labels[i] = CATEGORIES[i].label;

  int category_index = widget_run_menu(
    "ADD PART",
    category_labels,
    CATEGORY_COUNT,
    board->menu_category_index
  );

  if (category_index < 0)
    return;

  const PlacementCategory *category = &CATEGORIES[category_index];
  const char *entry_labels[CATEGORY_ENTRY_CAPACITY];

  for (int i = 0; i < category->entry_count; i++)
    entry_labels[i] = category->entries[i].label;

  int initial_entry_index =
    category_index == board->menu_category_index ? board->menu_entry_index : 0;

  int entry_index = widget_run_menu(
    category->label,
    entry_labels,
    category->entry_count,
    initial_entry_index
  );

  if (entry_index < 0)
    return;

  board->menu_category_index = category_index;
  board->menu_entry_index = entry_index;
  board->placing_entry = &category->entries[entry_index];
  board->placing_first_hole_index = NO_HOLE_INDEX;
}

void
cancel_placement(Breadboard *board) {
  board->placing_entry = NULL;
  board->placing_first_hole_index = NO_HOLE_INDEX;
}

int
compute_placement_hole_indices(const Breadboard *board, int *hole_indices) {
  PartKind kind = board->placing_entry->kind;

  if (has_footprint(kind)) {
    if (!compute_footprint_hole_indices(
          kind,
          board->cursor_row,
          board->cursor_column,
          hole_indices
        ))
      return 0;

    return count_terminals(kind);
  }

  if (board->placing_first_hole_index == NO_HOLE_INDEX) {
    hole_indices[0] = find_cursor_hole_index(board);
    return 1;
  }

  hole_indices[0] = board->placing_first_hole_index;
  hole_indices[1] = find_cursor_hole_index(board);
  return 2;
}

bool
is_placement_allowed(Breadboard *board) {
  int hole_indices[PART_TERMINAL_CAPACITY];
  int hole_count = compute_placement_hole_indices(board, hole_indices);

  return hole_count > 0 && are_holes_free(board, hole_indices, hole_count);
}

static void
confirm_placement(Breadboard *board) {
  if (!is_placement_allowed(board)) {
    snprintf(board->message, sizeof(board->message), "blocked");
    return;
  }

  PartKind kind = board->placing_entry->kind;

  if (!has_footprint(kind) &&
      board->placing_first_hole_index == NO_HOLE_INDEX) {
    board->placing_first_hole_index = find_cursor_hole_index(board);
    return;
  }

  int hole_indices[PART_TERMINAL_CAPACITY];

  compute_placement_hole_indices(board, hole_indices);
  add_part(board, kind, hole_indices, board->placing_entry->color_index);
  board->placing_first_hole_index = NO_HOLE_INDEX;
}

void
handle_placement_key(Breadboard *board, int key) {
  switch (key) {
  case KEY_ENTER:
  case ' ':
    confirm_placement(board);
    break;
  case KEY_ESCAPE:
  case 'q':
    cancel_placement(board);
    break;
  default:
    break;
  }
}

void
format_placement_title(const Breadboard *board, char *text, size_t text_size) {

  PartKind kind = board->placing_entry->kind;
  const char *label = board->placing_entry->label;

  if (has_footprint(kind)) {
    snprintf(text, text_size, "%s: place", label);
    return;
  }

  int terminal_index = board->placing_first_hole_index == NO_HOLE_INDEX ? 0 : 1;

  snprintf(
    text,
    text_size,
    "%s: %s",
    label,
    find_terminal_name(kind, terminal_index)
  );
}
