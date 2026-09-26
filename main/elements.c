#include "breadboard.h"

#include <stdio.h>

#define SWITCH_CLOSED_RESISTANCE 0.01
#define UNCONFIGURED_RESISTANCE 0.01
#define MOTOR_RESISTANCE 25.0
#define LED_SERIES_RESISTANCE 15.0
#define RED_LED_FORWARD_VOLTAGE 1.8
#define BLUE_LED_FORWARD_VOLTAGE 3.0
#define SEVEN_SEGMENT_COUNT 8
#define SEVEN_SEGMENT_COMMON_TERMINAL 2

static void
add_part_element(Breadboard *board, Part *part, CircuitElement element) {
  int element_index = circuit_add(&board->circuit, element);

  if (element_index < 0) {
    snprintf(board->message, sizeof(board->message), "No memory");
    return;
  }

  part->elements[part->element_count++] = element_index;
}

static void
add_part_resistor(
  Breadboard *board,
  Part *part,
  int first_node,
  int second_node,
  double resistance
) {

  CircuitElement element = {0};

  element.kind = CIRCUIT_RESISTOR;
  element.pins[0] = first_node;
  element.pins[1] = second_node;
  element.resistance = resistance;
  add_part_element(board, part, element);
}

static void
add_part_diode(
  Breadboard *board,
  Part *part,
  int anode_terminal,
  int cathode_terminal,
  double forward_voltage,
  double resistance
) {

  CircuitElement element = {0};

  element.kind = CIRCUIT_DIODE;
  element.pins[0] = part->terminal_node_indices[anode_terminal];
  element.pins[1] = part->terminal_node_indices[cathode_terminal];
  element.voltage = forward_voltage;
  element.resistance = resistance;
  add_part_element(board, part, element);
}

static void
add_part_source(
  Breadboard *board,
  Part *part,
  double voltage,
  double resistance
) {

  CircuitElement element = {0};

  element.kind = CIRCUIT_SOURCE;
  element.pins[0] = part->terminal_node_indices[0];
  element.pins[1] = part->terminal_node_indices[1];
  element.voltage = voltage;
  element.resistance = resistance;
  add_part_element(board, part, element);
}

static void
add_part_capacitor(Breadboard *board, Part *part, double capacitance) {
  CircuitElement element = {0};

  element.kind = CIRCUIT_CAPACITOR;
  element.pins[0] = part->terminal_node_indices[0];
  element.pins[1] = part->terminal_node_indices[1];
  element.value = capacitance;
  element.voltage = part->capacitor_voltage;
  add_part_element(board, part, element);
}

static void
add_part_npn(Breadboard *board, Part *part) {
  CircuitElement element = {0};

  element.kind = CIRCUIT_NPN;
  element.pins[0] = part->terminal_node_indices[2];
  element.pins[1] = part->terminal_node_indices[0];
  element.pins[2] = part->terminal_node_indices[1];
  element.value = 100.0;
  element.voltage = 0.7;
  element.resistance = 10.0;
  element.saturation_voltage = 0.2;
  element.saturation_resistance = 1.0;
  add_part_element(board, part, element);
}

static void
build_part_elements(Breadboard *board, Part *part) {
  const int16_t *nodes = part->terminal_node_indices;

  switch (part->kind) {
  case PART_KIND_RESISTOR:
  case PART_KIND_CDS:
    add_part_resistor(board, part, nodes[0], nodes[1], UNCONFIGURED_RESISTANCE);
    break;
  case PART_KIND_TACT_SWITCH:
    add_part_resistor(
      board,
      part,
      nodes[0],
      nodes[1],
      SWITCH_CLOSED_RESISTANCE
    );
    break;
  case PART_KIND_MOTOR:
    add_part_resistor(board, part, nodes[0], nodes[1], MOTOR_RESISTANCE);
    break;
  case PART_KIND_LED:
    add_part_diode(
      board,
      part,
      0,
      1,
      find_led_forward_voltage(part->color_index),
      LED_SERIES_RESISTANCE
    );
    break;
  case PART_KIND_RGB_LED:
    add_part_diode(
      board,
      part,
      0,
      1,
      RED_LED_FORWARD_VOLTAGE,
      LED_SERIES_RESISTANCE
    );
    add_part_diode(
      board,
      part,
      2,
      1,
      BLUE_LED_FORWARD_VOLTAGE,
      LED_SERIES_RESISTANCE
    );
    add_part_diode(
      board,
      part,
      3,
      1,
      BLUE_LED_FORWARD_VOLTAGE,
      LED_SERIES_RESISTANCE
    );
    break;
  case PART_KIND_SEVEN_SEGMENT:
    for (int i = 0; i < SEVEN_SEGMENT_COUNT; i++) {
      add_part_diode(
        board,
        part,
        find_seven_segment_anode_terminal(i),
        SEVEN_SEGMENT_COMMON_TERMINAL,
        RED_LED_FORWARD_VOLTAGE,
        LED_SERIES_RESISTANCE
      );
    }
    break;
  case PART_KIND_DIODE:
    add_part_diode(board, part, 0, 1, 0.7, 0.5);
    break;
  case PART_KIND_SLIDE_SWITCH:
    add_part_resistor(
      board,
      part,
      nodes[1],
      nodes[0],
      SWITCH_CLOSED_RESISTANCE
    );
    add_part_resistor(
      board,
      part,
      nodes[1],
      nodes[2],
      SWITCH_CLOSED_RESISTANCE
    );
    break;
  case PART_KIND_BATTERY:
    add_part_source(board, part, 5.0, 0.5);
    break;
  case PART_KIND_BUZZER:
    add_part_diode(board, part, 0, 1, 1.0, 150.0);
    break;
  case PART_KIND_NPN:
    add_part_npn(board, part);
    break;
  case PART_KIND_RELAY:
    add_part_resistor(board, part, nodes[0], nodes[2], 100.0);
    add_part_resistor(
      board,
      part,
      nodes[1],
      nodes[3],
      SWITCH_CLOSED_RESISTANCE
    );
    add_part_resistor(
      board,
      part,
      nodes[1],
      nodes[4],
      SWITCH_CLOSED_RESISTANCE
    );
    break;
  case PART_KIND_VOLUME:
    add_part_resistor(board, part, nodes[0], nodes[1], 1.0);
    add_part_resistor(board, part, nodes[1], nodes[2], 1.0);
    break;
  case PART_KIND_CAPACITOR:
    add_part_capacitor(board, part, 0.001);
    break;
  case PART_KIND_PINO:
    build_pino_elements(board, part);
    break;
  default:
    break;
  }
}

void
rebuild_circuit(Breadboard *board) {
  circuit_clear(&board->circuit);

  for (int i = 0; i < board->part_count; i++) {
    Part *part = &board->parts[i];
    int contact_pairs[CONTACT_PAIR_CAPACITY][2];
    int contact_pair_count =
      list_contact_terminal_pairs(part->kind, contact_pairs);

    for (int j = 0; j < contact_pair_count; j++) {
      circuit_connect(
        &board->circuit,
        part->terminal_node_indices[contact_pairs[j][0]],
        part->terminal_node_indices[contact_pairs[j][1]]
      );
    }

    part->element_count = 0;
    build_part_elements(board, part);
  }
}

static CircuitElement *
find_part_element(Breadboard *board, const Part *part, int element_slot) {
  if (element_slot >= part->element_count)
    return NULL;

  return &board->circuit.elements[part->elements[element_slot]];
}

static void
set_element_enabled(
  Breadboard *board,
  const Part *part,
  int element_slot,
  bool enabled
) {

  CircuitElement *element = find_part_element(board, part, element_slot);

  if (element == NULL)
    return;

  element->enabled = enabled;

  if (!enabled) {
    element->conducting = false;
    element->mode = 0;
  }
}

static void
set_element_resistance(
  Breadboard *board,
  const Part *part,
  int element_slot,
  double resistance
) {

  CircuitElement *element = find_part_element(board, part, element_slot);

  if (element == NULL)
    return;

  element->resistance = resistance;
}

void
configure_part_elements(Breadboard *board, Part *part) {
  switch (part->kind) {
  case PART_KIND_RESISTOR:
    set_element_resistance(
      board,
      part,
      0,
      find_resistor_value(part->resistance_index)
    );
    break;
  case PART_KIND_LED:
  case PART_KIND_RGB_LED:
  case PART_KIND_SEVEN_SEGMENT:
    for (int i = 0; i < part->element_count; i++)
      set_element_enabled(board, part, i, !part->burned);
    break;
  case PART_KIND_TACT_SWITCH:
    set_element_enabled(board, part, 0, part->pressed_frames > 0);
    break;
  case PART_KIND_SLIDE_SWITCH:
    set_element_enabled(board, part, 0, part->slide_position == 0);
    set_element_enabled(board, part, 1, part->slide_position == 1);
    break;
  case PART_KIND_RELAY:
    set_element_enabled(board, part, 1, !part->energized);
    set_element_enabled(board, part, 2, part->energized);
    break;
  case PART_KIND_VOLUME:
    set_element_resistance(board, part, 0, 1000.0 * part->knob_position + 0.01);
    set_element_resistance(
      board,
      part,
      1,
      1000.0 * (KNOB_POSITION_MAXIMUM - part->knob_position) + 0.01
    );
    break;
  case PART_KIND_CDS:
    set_element_resistance(
      board,
      part,
      0,
      200000.0 / (double)(1 << part->light_level)
    );
    break;
  case PART_KIND_PINO:
    configure_pino_elements(board, part);
    break;
  default:
    break;
  }
}
