#include "breadboard.h"

#include <math.h>
#include <stdio.h>

#define SIMULATION_STEP_SECONDS 0.05
#define SOLVER_ITERATION_COUNT 12
#define SHORT_CIRCUIT_CURRENT 2.0
#define RELAY_ENERGIZE_CURRENT 0.03
#define LED_BURNOUT_CURRENT 0.05
#define LED_FULL_BRIGHTNESS_CURRENT 0.02
#define LED_VISIBLE_CURRENT 0.0005
#define BUZZER_SOUND_CURRENT 0.005
#define BUZZER_FULL_VOLUME_CURRENT 0.025
#define CAPACITOR_FULL_VOLTAGE 5.0
#define MOTOR_PHASE_STEP_PER_AMPERE 20.0
#define MOTOR_PHASE_STEP_MAXIMUM (MOTOR_PHASE_COUNT / 4)

static void
count_down_tact_switches(Breadboard *board) {
  for (int i = 0; i < board->part_count; i++) {
    Part *part = &board->parts[i];

    if (part->kind != PART_KIND_TACT_SWITCH || part->pressed_frames <= 0)
      continue;

    part->pressed_frames--;

    if (part->pressed_frames == 0) {
      board->circuit_dirty = true;
      board->needs_redraw = true;
    }
  }
}

static bool
has_junction_currents(PartKind kind) {
  switch (kind) {
  case PART_KIND_LED:
  case PART_KIND_RGB_LED:
  case PART_KIND_SEVEN_SEGMENT:
  case PART_KIND_DIODE:
  case PART_KIND_BUZZER:
    return true;
  default:
    return false;
  }
}

static bool
is_led_kind(PartKind kind) {
  return kind == PART_KIND_LED || kind == PART_KIND_RGB_LED ||
         kind == PART_KIND_SEVEN_SEGMENT;
}

static double
read_element_current(const Breadboard *board, int element_index) {
  return board->circuit.elements[element_index].current;
}

static void
record_junction_currents(const Breadboard *board, Part *part) {
  if (!has_junction_currents(part->kind))
    return;

  for (int i = 0; i < part->element_count; i++)
    part->junction_currents[i] = read_element_current(board, part->elements[i]);

  part->junction_current_count = part->element_count;
}

static double
find_largest_junction_current(const Part *part) {
  double largest_current = -INFINITY;

  for (int i = 0; i < part->junction_current_count; i++) {
    if (part->junction_currents[i] > largest_current)
      largest_current = part->junction_currents[i];
  }

  return largest_current;
}

static void
burn_overloaded_led(Breadboard *board, Part *part) {
  if (!is_led_kind(part->kind) || part->burned)
    return;

  if (find_largest_junction_current(part) <= LED_BURNOUT_CURRENT)
    return;

  part->burned = true;
  board->circuit_dirty = true;
}

static int
compute_brightness_level(double current) {
  if (current < LED_VISIBLE_CURRENT)
    return 0;

  int level = (int)(current / LED_FULL_BRIGHTNESS_CURRENT * LEVEL_MAXIMUM);

  return clamp_integer(level, 1, LEVEL_MAXIMUM);
}

static int
compute_volume_level(double current) {
  if (current <= BUZZER_SOUND_CURRENT)
    return 0;

  int level = (int)(current / BUZZER_FULL_VOLUME_CURRENT * LEVEL_MAXIMUM);

  return clamp_integer(level, 1, LEVEL_MAXIMUM);
}

static int
compute_part_levels(const Part *part, int *levels) {
  switch (part->kind) {
  case PART_KIND_LED:
  case PART_KIND_RGB_LED:
  case PART_KIND_SEVEN_SEGMENT:
    for (int i = 0; i < part->junction_current_count; i++)
      levels[i] = compute_brightness_level(part->junction_currents[i]);
    return part->junction_current_count;
  case PART_KIND_BUZZER:
    levels[0] = compute_volume_level(part->junction_currents[0]);
    return 1;
  case PART_KIND_CAPACITOR:
    levels[0] = clamp_integer(
      (int)(fabs(part->capacitor_voltage) / CAPACITOR_FULL_VOLTAGE *
            LEVEL_MAXIMUM),
      0,
      LEVEL_MAXIMUM
    );
    return 1;
  case PART_KIND_RELAY:
    levels[0] = part->energized ? 1 : 0;
    return 1;
  default:
    for (int i = 0; i < part->level_count; i++)
      levels[i] = part->levels[i];
    return part->level_count;
  }
}

static bool
are_levels_equal(const Part *part, const int *levels, int level_count) {
  if (level_count != part->level_count)
    return false;

  for (int i = 0; i < level_count; i++) {
    if (levels[i] != part->levels[i])
      return false;
  }

  return true;
}

static void
update_part_levels(Breadboard *board, Part *part) {
  int levels[PART_ELEMENT_CAPACITY];
  int level_count = compute_part_levels(part, levels);

  if (!are_levels_equal(part, levels, level_count))
    board->needs_redraw = true;

  for (int i = 0; i < level_count; i++)
    part->levels[i] = levels[i];

  part->level_count = level_count;
}

static bool
has_main_current(PartKind kind) {
  switch (kind) {
  case PART_KIND_RESISTOR:
  case PART_KIND_BATTERY:
  case PART_KIND_MOTOR:
  case PART_KIND_NPN:
  case PART_KIND_RELAY:
  case PART_KIND_CDS:
  case PART_KIND_CAPACITOR:
    return true;
  default:
    return false;
  }
}

static void
update_part_from_solution(Breadboard *board, Part *part) {
  const int16_t *nodes = part->terminal_node_indices;

  part->voltage = circuit_voltage(&board->circuit, nodes[0], nodes[1]);

  if (has_main_current(part->kind) && part->element_count > 0)
    part->current = read_element_current(board, part->elements[0]);

  record_junction_currents(board, part);
  burn_overloaded_led(board, part);

  switch (part->kind) {
  case PART_KIND_BATTERY:
    if (part->current > SHORT_CIRCUIT_CURRENT)
      board->short_circuit = true;
    break;
  case PART_KIND_CAPACITOR:
    part->capacitor_voltage = part->voltage;
    break;
  case PART_KIND_RELAY: {
    bool energized = fabs(part->current) > RELAY_ENERGIZE_CURRENT;

    if (energized != part->energized)
      board->circuit_dirty = true;

    part->energized = energized;
    break;
  }
  case PART_KIND_PINO:
    read_pino_solution(board, part);
    break;
  default:
    break;
  }

  update_part_levels(board, part);
}

void
solve_circuit(Breadboard *board) {
  board->circuit_dirty = false;

  for (int i = 0; i < board->part_count; i++)
    configure_part_elements(board, &board->parts[i]);

  if (!circuit_step(
        &board->circuit,
        SIMULATION_STEP_SECONDS,
        SOLVER_ITERATION_COUNT
      )) {

    snprintf(board->message, sizeof(board->message), "No memory");
    return;
  }

  board->short_circuit = false;

  for (int i = 0; i < board->part_count; i++)
    update_part_from_solution(board, &board->parts[i]);
}

int
compute_motor_phase_step(const Part *part) {
  return clamp_integer(
    (int)(part->current * MOTOR_PHASE_STEP_PER_AMPERE),
    -MOTOR_PHASE_STEP_MAXIMUM,
    MOTOR_PHASE_STEP_MAXIMUM
  );
}

static void
advance_motors(Breadboard *board) {
  for (int i = 0; i < board->part_count; i++) {
    Part *part = &board->parts[i];

    if (part->kind != PART_KIND_MOTOR)
      continue;

    part->motor_phase =
      (part->motor_phase + compute_motor_phase_step(part) + MOTOR_PHASE_COUNT) %
      MOTOR_PHASE_COUNT;
  }
}

void
step_simulation(Breadboard *board) {
  count_down_tact_switches(board);

  if (board->circuit_dirty || board->time_dependent)
    solve_circuit(board);

  advance_motors(board);
}

bool
is_animation_running(const Breadboard *board) {
  for (int i = 0; i < board->part_count; i++) {
    const Part *part = &board->parts[i];

    if (part->kind == PART_KIND_MOTOR && compute_motor_phase_step(part) != 0)
      return true;

    if (part->kind == PART_KIND_BUZZER && part->level_count > 0 &&
        part->levels[0] > 0)
      return true;
  }

  return false;
}
