#include "breadboard.h"

#include "canvas.h"

#include <math.h>

#define LEAD_COLOR 0x8A8A8A
#define RESISTOR_BODY_COLOR 0xD8B070
#define RESISTOR_BAND_COLOR 0x803010
#define DARK_BODY_COLOR 0x202020
#define TERMINAL_BAND_COLOR 0xB0B0B0
#define BURNED_COLOR 0x282828
#define UNLIT_LEVEL (LEVEL_MAXIMUM / 4)
#define SOUND_COLOR 0xA04800
#define MOTOR_BODY_COLOR 0xB0B0B0
#define MOTOR_PHASE_RADIANS 0.39269908169872414
#define CDS_BODY_COLOR 0xD08040
#define CAPACITOR_BODY_COLOR 0x2050B0
#define CHARGE_COLOR 0xFFD020

static void
draw_wire(int start_x, int start_y, int end_x, int end_y, uint32_t color) {
  canvas_line(start_x, start_y, end_x, end_y, color);
  canvas_line(start_x + 1, start_y, end_x + 1, end_y, color);
  canvas_line(start_x, start_y + 1, end_x, end_y + 1, color);
  canvas_fill_rect(start_x - 1, start_y - 1, 3, 3, color);
  canvas_fill_rect(end_x - 1, end_y - 1, 3, 3, color);
}

static int
compute_sign(int value) {
  if (value > 0)
    return 1;

  if (value < 0)
    return -1;

  return 0;
}

static void
draw_terminal_band(int middle_x, int middle_y, int toward_x, int toward_y) {
  int band_x = middle_x + compute_sign(toward_x - middle_x) * 2;
  int band_y = middle_y + compute_sign(toward_y - middle_y) * 2;

  canvas_fill_rect(band_x - 1, band_y - 1, 3, 3, TERMINAL_BAND_COLOR);
}

static int
read_first_level(const Part *part) {
  return part->level_count > 0 ? part->levels[0] : 0;
}

static void
draw_led(const Part *part, int center_x, int center_y) {
  uint32_t bright_color = find_led_color(part->color_index);

  if (part->burned) {
    canvas_fill_circle(center_x, center_y, 3, BURNED_COLOR);
    canvas_line(
      center_x - 2,
      center_y - 2,
      center_x + 2,
      center_y + 2,
      bright_color
    );
    canvas_line(
      center_x - 2,
      center_y + 2,
      center_x + 2,
      center_y - 2,
      bright_color
    );
    return;
  }

  uint32_t unlit_color = blend_color(BURNED_COLOR, bright_color, UNLIT_LEVEL);
  int level = read_first_level(part);

  canvas_fill_circle(
    center_x,
    center_y,
    3,
    blend_color(unlit_color, bright_color, level)
  );

  if (level == LEVEL_MAXIMUM)
    canvas_circle(center_x, center_y, 5, bright_color);
}

static void
draw_buzzer(const Part *part, int frame_count, int center_x, int center_y) {
  canvas_fill_circle(center_x, center_y, 4, DARK_BODY_COLOR);

  int level = read_first_level(part);

  if (level == 0)
    return;

  canvas_circle(
    center_x,
    center_y,
    6 + (frame_count % 3) * level / 2,
    SOUND_COLOR
  );
}

static void
draw_motor(const Part *part, int center_x, int center_y) {
  double angle = part->motor_phase * MOTOR_PHASE_RADIANS;
  int tip_x = center_x + (int)(cos(angle) * 4);
  int tip_y = center_y + (int)(sin(angle) * 4);

  canvas_fill_circle(center_x, center_y, 5, MOTOR_BODY_COLOR);
  canvas_line(center_x, center_y, tip_x, tip_y, DARK_BODY_COLOR);
}

static void
draw_capacitor(const Part *part, int center_x, int center_y) {
  canvas_fill_circle(center_x, center_y, 4, CAPACITOR_BODY_COLOR);

  int level = read_first_level(part);

  if (level == 0)
    return;

  int charge_height = level * 6 / LEVEL_MAXIMUM;

  canvas_fill_rect(
    center_x - 1,
    center_y + 3 - charge_height,
    3,
    charge_height,
    CHARGE_COLOR
  );
}

static void
draw_lead_part(const Part *part, int frame_count) {
  int start_x = compute_hole_x(part->terminal_hole_indices[0]);
  int start_y = compute_hole_y(part->terminal_hole_indices[0]);
  int end_x = compute_hole_x(part->terminal_hole_indices[1]);
  int end_y = compute_hole_y(part->terminal_hole_indices[1]);
  int middle_x = (start_x + end_x) / 2;
  int middle_y = (start_y + end_y) / 2;

  if (part->kind == PART_KIND_WIRE) {
    draw_wire(
      start_x,
      start_y,
      end_x,
      end_y,
      find_wire_color(part->color_index)
    );
    return;
  }

  canvas_line(start_x, start_y, end_x, end_y, LEAD_COLOR);

  switch (part->kind) {
  case PART_KIND_RESISTOR:
    canvas_fill_rect(middle_x - 4, middle_y - 2, 9, 5, RESISTOR_BODY_COLOR);
    canvas_fill_rect(middle_x - 2, middle_y - 2, 1, 5, RESISTOR_BAND_COLOR);
    canvas_fill_rect(middle_x + 1, middle_y - 2, 1, 5, RESISTOR_BAND_COLOR);
    break;
  case PART_KIND_LED:
    draw_led(part, middle_x, middle_y);
    break;
  case PART_KIND_DIODE:
    canvas_fill_rect(middle_x - 3, middle_y - 2, 7, 5, DARK_BODY_COLOR);
    draw_terminal_band(middle_x, middle_y, end_x, end_y);
    break;
  case PART_KIND_BATTERY:
    canvas_fill_rect(middle_x - 6, middle_y - 3, 13, 7, DARK_BODY_COLOR);
    draw_terminal_band(middle_x, middle_y, start_x, start_y);
    break;
  case PART_KIND_BUZZER:
    draw_buzzer(part, frame_count, middle_x, middle_y);
    break;
  case PART_KIND_MOTOR:
    draw_motor(part, middle_x, middle_y);
    break;
  case PART_KIND_CDS:
    canvas_fill_circle(middle_x, middle_y, 3, CDS_BODY_COLOR);
    canvas_line(
      middle_x - 2,
      middle_y,
      middle_x + 2,
      middle_y,
      RESISTOR_BAND_COLOR
    );
    break;
  case PART_KIND_CAPACITOR:
    draw_capacitor(part, middle_x, middle_y);
    break;
  default:
    break;
  }
}

void
draw_parts(const Breadboard *board) {
  for (int i = 0; i < board->part_count; i++) {
    const Part *part = &board->parts[i];

    if (has_footprint(part->kind))
      draw_module_part(part);
    else
      draw_lead_part(part, board->frame_count);
  }
}
