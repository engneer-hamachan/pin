#pragma once

#include "circuit.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define BOARD_ROW_COUNT 14
#define BOARD_COLUMN_COUNT 30
#define HOLE_COUNT (BOARD_ROW_COUNT * BOARD_COLUMN_COUNT)
#define NET_COUNT 64
#define PART_CAPACITY (HOLE_COUNT / 2)
#define PART_TERMINAL_CAPACITY 10
#define PART_ELEMENT_CAPACITY 8
#define CONTACT_PAIR_CAPACITY 2
#define RESISTOR_VALUE_COUNT 10
#define KNOB_POSITION_MAXIMUM 10
#define LIGHT_LEVEL_MAXIMUM 10
#define LEVEL_MAXIMUM 8
#define MOTOR_PHASE_COUNT 16
#define WIRE_COLOR_COUNT 6
#define MESSAGE_SIZE 32
#define TEXT_SIZE 64
#define SAVE_SLOT_COUNT 4
#define SD_MOUNT_PATH "/sdcard"
#define SAVE_DIRECTORY_PATH SD_MOUNT_PATH "/Pin_data"

typedef enum {
  PART_KIND_WIRE,
  PART_KIND_RESISTOR,
  PART_KIND_LED,
  PART_KIND_RGB_LED,
  PART_KIND_SEVEN_SEGMENT,
  PART_KIND_DIODE,
  PART_KIND_TACT_SWITCH,
  PART_KIND_SLIDE_SWITCH,
  PART_KIND_BATTERY,
  PART_KIND_BUZZER,
  PART_KIND_MOTOR,
  PART_KIND_NPN,
  PART_KIND_RELAY,
  PART_KIND_VOLUME,
  PART_KIND_CDS,
  PART_KIND_CAPACITOR
} PartKind;

typedef struct {
  PartKind kind;
  int terminal_count;
  int terminal_hole_indices[PART_TERMINAL_CAPACITY];
  int terminal_node_indices[PART_TERMINAL_CAPACITY];
  int element_count;
  int elements[PART_ELEMENT_CAPACITY];
  int junction_current_count;
  double junction_currents[PART_ELEMENT_CAPACITY];
  int level_count;
  int levels[PART_ELEMENT_CAPACITY];
  int color_index;
  int resistance_index;
  int knob_position;
  int light_level;
  int slide_position;
  int pressed_frames;
  int motor_phase;
  bool energized;
  bool burned;
  double capacitor_voltage;
  double current;
  double voltage;
} Part;

typedef struct {
  const char *label;
  PartKind kind;
  int color_index;
} PlacementEntry;

typedef struct {
  Part parts[PART_CAPACITY];
  int part_count;
  Circuit circuit;
  int cursor_row;
  int cursor_column;
  const PlacementEntry *placing_entry;
  int placing_first_hole_index;
  int menu_category_index;
  int menu_entry_index;
  char message[MESSAGE_SIZE];
  bool help_visible;
  bool quit;
  bool needs_redraw;
  bool circuit_dirty;
  bool time_dependent;
  bool short_circuit;
  int frame_count;
} Breadboard;

int build_hole_index(int row, int column);
int read_hole_row(int hole_index);
int read_hole_column(int hole_index);
bool hole_exists(int row, int column);
bool is_rail_hole(int hole_index);
int compute_row_y(int row);
int compute_column_x(int column);
int compute_hole_x(int hole_index);
int compute_hole_y(int hole_index);
int find_base_net(int hole_index);
void format_hole_name(int hole_index, char *text, size_t text_size);
int find_cursor_hole_index(const Breadboard *board);
void move_cursor(Breadboard *board, int column_step, int row_step);

const char *find_part_name(PartKind kind);
const char *find_terminal_name(PartKind kind, int terminal_index);

bool has_footprint(PartKind kind);
int count_terminals(PartKind kind);
int list_contact_terminal_pairs(
  PartKind kind,
  int contact_pairs[CONTACT_PAIR_CAPACITY][2]
);
bool compute_footprint_hole_indices(
  PartKind kind,
  int anchor_row,
  int anchor_column,
  int *hole_indices
);

int find_resistor_value(int resistance_index);
double find_led_forward_voltage(int color_index);
uint32_t find_led_color(int color_index);
uint32_t find_wire_color(int color_index);
const char *find_rail_name(int net);
int find_seven_segment_anode_terminal(int segment_index);

Part *find_part_at(Breadboard *board, int hole_index);
bool are_holes_free(Breadboard *board, const int *hole_indices, int hole_count);
int count_parts_of_kind(const Breadboard *board, PartKind kind);
void add_part(
  Breadboard *board,
  PartKind kind,
  const int *terminal_hole_indices,
  int color_index
);
void remove_part(Breadboard *board, Part *part);
void remove_all_parts(Breadboard *board);
void update_circuit_structure(Breadboard *board);
void operate_part(Breadboard *board, Part *part);
void adjust_part(Breadboard *board, Part *part, int step);
int clamp_integer(int value, int minimum, int maximum);

void rebuild_circuit(Breadboard *board);
void configure_part_elements(Breadboard *board, Part *part);

void step_simulation(Breadboard *board);
bool is_animation_running(const Breadboard *board);
int compute_motor_phase_step(const Part *part);

int count_placement_entries(void);
const PlacementEntry *find_placement_entry(int entry_index);
void start_placement(Breadboard *board);
void cancel_placement(Breadboard *board);
void handle_placement_key(Breadboard *board, int key);
int compute_placement_hole_indices(const Breadboard *board, int *hole_indices);
bool is_placement_allowed(Breadboard *board);
void
format_placement_title(const Breadboard *board, char *text, size_t text_size);

int decode_key(int key);
bool move_cursor_by_key(Breadboard *board, int key);
void handle_key(Breadboard *board, int key);

bool mount_sd(void);
void unmount_sd(void);

extern const char *const SAVE_FILE_NAMES[SAVE_SLOT_COUNT];

bool save_board(const Breadboard *board, int slot_index);
bool load_board(Breadboard *board, int slot_index);

void format_part_reading(const Part *part, char *text, size_t text_size);
void format_header_title(Breadboard *board, char *text, size_t text_size);
void format_header_right(Breadboard *board, char *text, size_t text_size);

void draw_board(void);
void draw_placement_preview(Breadboard *board);
void draw_cursor(const Breadboard *board);
void draw_screen(Breadboard *board);
uint32_t blend_color(uint32_t dark_color, uint32_t bright_color, int level);
void draw_parts(const Breadboard *board);
void draw_module_part(const Part *part);
