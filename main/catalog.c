#include "breadboard.h"

const char *
find_part_name(PartKind kind) {
  switch (kind) {
  case PART_KIND_WIRE:
    return "Wire";
  case PART_KIND_RESISTOR:
    return "Resistor";
  case PART_KIND_LED:
    return "LED";
  case PART_KIND_RGB_LED:
    return "RGB LED";
  case PART_KIND_SEVEN_SEGMENT:
    return "7-seg";
  case PART_KIND_DIODE:
    return "Diode";
  case PART_KIND_TACT_SWITCH:
    return "Tact SW";
  case PART_KIND_SLIDE_SWITCH:
    return "Slide SW";
  case PART_KIND_BATTERY:
    return "Battery";
  case PART_KIND_BUZZER:
    return "Buzzer";
  case PART_KIND_MOTOR:
    return "Motor";
  case PART_KIND_NPN:
    return "NPN Tr";
  case PART_KIND_RELAY:
    return "Relay";
  case PART_KIND_VOLUME:
    return "Volume";
  case PART_KIND_CDS:
    return "CdS";
  default:
    return "Capacitor";
  }
}

static const char *
find_rgb_terminal_name(int terminal_index) {
  switch (terminal_index) {
  case 0:
    return "red";
  case 1:
    return "common-";
  case 2:
    return "green";
  default:
    return "blue";
  }
}

static const char *
find_seven_segment_terminal_name(int terminal_index) {
  switch (terminal_index) {
  case 0:
    return "g";
  case 1:
    return "f";
  case 3:
    return "a";
  case 4:
    return "b";
  case 5:
    return "e";
  case 6:
    return "d";
  case 8:
    return "c";
  case 9:
    return "dp";
  default:
    return "common-";
  }
}

static const char *
find_transistor_terminal_name(int terminal_index) {
  switch (terminal_index) {
  case 0:
    return "emitter";
  case 1:
    return "base";
  default:
    return "collector";
  }
}

static const char *
find_relay_terminal_name(int terminal_index) {
  switch (terminal_index) {
  case 1:
    return "common";
  case 3:
    return "NC";
  case 4:
    return "NO";
  default:
    return "coil";
  }
}

const char *
find_terminal_name(PartKind kind, int terminal_index) {
  switch (kind) {
  case PART_KIND_WIRE:
    return "end";
  case PART_KIND_LED:
  case PART_KIND_DIODE:
    return terminal_index == 0 ? "anode+" : "cathode-";
  case PART_KIND_BATTERY:
  case PART_KIND_BUZZER:
  case PART_KIND_CAPACITOR:
    return terminal_index == 0 ? "+" : "-";
  case PART_KIND_RGB_LED:
    return find_rgb_terminal_name(terminal_index);
  case PART_KIND_SEVEN_SEGMENT:
    return find_seven_segment_terminal_name(terminal_index);
  case PART_KIND_TACT_SWITCH:
    return terminal_index % 2 == 0 ? "left" : "right";
  case PART_KIND_SLIDE_SWITCH:
    return terminal_index == 1 ? "common" : (terminal_index == 0 ? "A" : "B");
  case PART_KIND_VOLUME:
    return terminal_index == 1 ? "wiper" : (terminal_index == 0 ? "A" : "B");
  case PART_KIND_NPN:
    return find_transistor_terminal_name(terminal_index);
  case PART_KIND_RELAY:
    return find_relay_terminal_name(terminal_index);
  default:
    return "lead";
  }
}
