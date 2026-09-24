#include "breadboard.h"

#include <math.h>
#include <stdio.h>

#define READING_PART_SIZE 24

static int
find_terminal_index(const Part *part, int hole_index) {
  for (int i = 0; i < part->terminal_count; i++) {
    if (part->terminal_hole_indices[i] == hole_index)
      return i;
  }

  return 0;
}

static void
format_current(double amperes, char *text, size_t text_size) {
  long long microamperes = (long long)(fabs(amperes) * 1000000);
  const char *sign = amperes < 0 ? "-" : "";

  if (microamperes < 1000)
    snprintf(text, text_size, "%s%llduA", sign, microamperes);
  else if (microamperes < 1000000)
    snprintf(text, text_size, "%s%lldmA", sign, microamperes / 1000);
  else
    snprintf(text, text_size, "%s%lldA", sign, microamperes / 1000000);
}

static void
format_voltage(double volts, char *text, size_t text_size) {
  int tenths = (int)(fabs(volts) * 10 + 0.5);
  const char *sign = volts < 0 ? "-" : "";

  snprintf(text, text_size, "%s%d.%dV", sign, tenths / 10, tenths % 10);
}

static void
format_resistance(int ohms, char *text, size_t text_size) {
  if (ohms < 1000)
    snprintf(text, text_size, "%dohm", ohms);
  else if (ohms % 1000 == 0)
    snprintf(text, text_size, "%dk", ohms / 1000);
  else
    snprintf(text, text_size, "%d.%dk", ohms / 1000, ohms % 1000 / 100);
}

static double
sum_junction_currents(const Part *part) {
  double sum = 0.0;

  for (int i = 0; i < part->junction_current_count; i++)
    sum += part->junction_currents[i];

  return sum;
}

static void
format_part_reading(const Part *part, char *text, size_t text_size) {
  char first_reading[READING_PART_SIZE];
  char second_reading[READING_PART_SIZE];

  switch (part->kind) {
  case PART_KIND_WIRE:
    text[0] = 0;
    break;
  case PART_KIND_RESISTOR:
    format_resistance(
      find_resistor_value(part->resistance_index),
      first_reading,
      sizeof first_reading
    );
    format_current(part->current, second_reading, sizeof second_reading);
    snprintf(text, text_size, "%s %s", first_reading, second_reading);
    break;
  case PART_KIND_LED:
  case PART_KIND_RGB_LED:
  case PART_KIND_SEVEN_SEGMENT:
    if (part->burned)
      snprintf(text, text_size, "BURNED");
    else
      format_current(sum_junction_currents(part), text, text_size);
    break;
  case PART_KIND_DIODE:
  case PART_KIND_BUZZER:
    format_current(
      part->junction_current_count > 0 ? part->junction_currents[0] : 0.0,
      text,
      text_size
    );
    break;
  case PART_KIND_NPN:
    format_current(part->current, first_reading, sizeof first_reading);
    snprintf(text, text_size, "Ic %s", first_reading);
    break;
  case PART_KIND_BATTERY:
    format_voltage(5.0, first_reading, sizeof first_reading);
    format_current(part->current, second_reading, sizeof second_reading);
    snprintf(text, text_size, "%s %s", first_reading, second_reading);
    break;
  case PART_KIND_CAPACITOR:
    format_voltage(part->capacitor_voltage, text, text_size);
    break;
  case PART_KIND_RELAY:
    snprintf(text, text_size, "%s", part->energized ? "ON" : "OFF");
    break;
  case PART_KIND_VOLUME:
    snprintf(
      text,
      text_size,
      "knob %d/%d",
      part->knob_position,
      KNOB_POSITION_MAXIMUM
    );
    break;
  case PART_KIND_CDS:
    snprintf(
      text,
      text_size,
      "light %d/%d",
      part->light_level,
      LIGHT_LEVEL_MAXIMUM
    );
    break;
  case PART_KIND_TACT_SWITCH:
    snprintf(
      text,
      text_size,
      "%s",
      part->pressed_frames > 0 ? "pressed" : "space"
    );
    break;
  case PART_KIND_SLIDE_SWITCH:
    snprintf(
      text,
      text_size,
      "%s",
      part->slide_position == 0 ? "A side" : "B side"
    );
    break;
  default:
    format_current(part->current, text, text_size);
    break;
  }
}

void
format_header_title(Breadboard *board, char *text, size_t text_size) {
  if (board->placing_entry) {
    format_placement_title(board, text, text_size);
    return;
  }

  int hole_index = find_cursor_hole_index(board);
  const Part *part = find_part_at(board, hole_index);

  if (part == NULL) {
    format_hole_name(hole_index, text, text_size);
    return;
  }

  snprintf(
    text,
    text_size,
    "%s %s",
    find_part_name(part->kind),
    find_terminal_name(part->kind, find_terminal_index(part, hole_index))
  );
}

void
format_header_right(Breadboard *board, char *text, size_t text_size) {
  if (board->message[0]) {
    snprintf(text, text_size, "%s", board->message);
    return;
  }

  if (board->short_circuit) {
    snprintf(text, text_size, "SHORT!");
    return;
  }

  const Part *part = board->placing_entry
                       ? NULL
                       : find_part_at(board, find_cursor_hole_index(board));

  if (part == NULL) {
    snprintf(text, text_size, "? help");
    return;
  }

  format_part_reading(part, text, text_size);
}
