#include "breadboard.h"

int
find_resistor_value(int resistance_index) {
  switch (resistance_index) {
  case 0:
    return 10;
  case 1:
    return 100;
  case 2:
    return 220;
  case 3:
    return 330;
  case 4:
    return 470;
  case 5:
    return 1000;
  case 6:
    return 2200;
  case 7:
    return 4700;
  case 8:
    return 10000;
  default:
    return 100000;
  }
}

double
find_led_forward_voltage(int color_index) {
  switch (color_index) {
  case 0:
    return 1.8;
  case 1:
    return 2.0;
  case 2:
    return 2.1;
  default:
    return 3.0;
  }
}

uint32_t
find_led_color(int color_index) {
  switch (color_index) {
  case 0:
    return 0xFF3020;
  case 1:
    return 0xFFD020;
  case 2:
    return 0x30FF40;
  default:
    return 0x3070FF;
  }
}

uint32_t
find_wire_color(int color_index) {
  switch (color_index) {
  case 0:
    return 0xE03030;
  case 1:
    return 0x3060E0;
  case 2:
    return 0x30A040;
  case 3:
    return 0xE0B020;
  case 4:
    return 0xE07020;
  default:
    return 0x303030;
  }
}

const char *
find_rail_name(int net) {
  switch (net) {
  case 0:
    return "top+";
  case 1:
    return "top-";
  case 2:
    return "bottom+";
  default:
    return "bottom-";
  }
}

int
find_seven_segment_anode_terminal(int segment_index) {
  switch (segment_index) {
  case 0:
    return 3;
  case 1:
    return 4;
  case 2:
    return 8;
  case 3:
    return 6;
  case 4:
    return 5;
  case 5:
    return 1;
  case 6:
    return 0;
  default:
    return 9;
  }
}
