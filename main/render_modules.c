#include "breadboard.h"

#include "canvas.h"

#define DARK_BODY_COLOR 0x202020
#define BLUE_BODY_COLOR 0x2050B0
#define METAL_COLOR 0xB0B0B0
#define ACTIVE_COLOR 0xFFD020
#define BURNED_COLOR 0x282828
#define SEGMENT_UNLIT_COLOR 0x401010
#define SEGMENT_LIT_COLOR 0xFF3020
#define RGB_CHANNEL_COUNT 3
#define SEVEN_SEGMENT_COUNT 8

typedef struct {
  int left;
  int top;
  int right;
  int bottom;
} Bounds;

static Bounds
compute_terminal_bounds(const Part *part) {
  Bounds bounds;

  bounds.left = compute_hole_x(part->terminal_hole_indices[0]);
  bounds.top = compute_hole_y(part->terminal_hole_indices[0]);
  bounds.right = bounds.left;
  bounds.bottom = bounds.top;

  for (int i = 1; i < part->terminal_count; i++) {
    int x = compute_hole_x(part->terminal_hole_indices[i]);
    int y = compute_hole_y(part->terminal_hole_indices[i]);

    if (x < bounds.left)
      bounds.left = x;

    if (x > bounds.right)
      bounds.right = x;

    if (y < bounds.top)
      bounds.top = y;

    if (y > bounds.bottom)
      bounds.bottom = y;
  }

  bounds.left -= 3;
  bounds.top -= 3;
  bounds.right += 3;
  bounds.bottom += 3;

  return bounds;
}

static int
find_largest_level(const Part *part) {
  int largest_level = 0;

  for (int i = 0; i < part->level_count; i++) {
    if (part->levels[i] > largest_level)
      largest_level = part->levels[i];
  }

  return largest_level;
}

static uint32_t
compute_rgb_color(const Part *part) {
  if (part->burned)
    return BURNED_COLOR;

  if (part->level_count < RGB_CHANNEL_COUNT || find_largest_level(part) == 0)
    return METAL_COLOR;

  uint32_t red = (uint32_t)(255 * part->levels[0] / LEVEL_MAXIMUM);
  uint32_t green = (uint32_t)(255 * part->levels[1] / LEVEL_MAXIMUM);
  uint32_t blue = (uint32_t)(255 * part->levels[2] / LEVEL_MAXIMUM);

  return (red << 16) | (green << 8) | blue;
}

static void
draw_knob(const Part *part, int center_x, int center_y) {
  int tip_x = center_x - 3 + part->knob_position * 6 / KNOB_POSITION_MAXIMUM;

  canvas_fill_circle(center_x, center_y, 3, METAL_COLOR);
  canvas_line(center_x, center_y, tip_x, center_y - 3, DARK_BODY_COLOR);
}

static void
draw_seven_segment(
  int segment_index,
  int center_x,
  int center_y,
  uint32_t color
) {
  switch (segment_index) {
  case 0:
    canvas_fill_rect(center_x - 4, center_y - 10, 9, 2, color);
    break;
  case 1:
    canvas_fill_rect(center_x + 5, center_y - 9, 2, 8, color);
    break;
  case 2:
    canvas_fill_rect(center_x + 5, center_y + 1, 2, 8, color);
    break;
  case 3:
    canvas_fill_rect(center_x - 4, center_y + 9, 9, 2, color);
    break;
  case 4:
    canvas_fill_rect(center_x - 6, center_y + 1, 2, 8, color);
    break;
  case 5:
    canvas_fill_rect(center_x - 6, center_y - 9, 2, 8, color);
    break;
  case 6:
    canvas_fill_rect(center_x - 4, center_y - 1, 9, 2, color);
    break;
  default:
    canvas_fill_rect(center_x + 8, center_y + 9, 2, 2, color);
    break;
  }
}

static void
draw_seven_segments(const Part *part, int center_x, int center_y) {
  for (int i = 0; i < SEVEN_SEGMENT_COUNT; i++) {
    int level = part->burned || part->level_count <= i ? 0 : part->levels[i];

    draw_seven_segment(
      i,
      center_x,
      center_y,
      blend_color(SEGMENT_UNLIT_COLOR, SEGMENT_LIT_COLOR, level)
    );
  }
}

static void
draw_module_pins(const Part *part) {
  for (int i = 0; i < part->terminal_count; i++) {
    canvas_pixel(
      compute_hole_x(part->terminal_hole_indices[i]),
      compute_hole_y(part->terminal_hole_indices[i]),
      METAL_COLOR
    );
  }
}

void
draw_module_part(const Part *part) {
  Bounds bounds = compute_terminal_bounds(part);
  int width = bounds.right - bounds.left + 1;
  int height = bounds.bottom - bounds.top + 1;
  int center_x = bounds.left + width / 2;
  int center_y = bounds.top + height / 2;

  switch (part->kind) {
  case PART_KIND_RGB_LED:
    canvas_fill_circle(center_x, center_y, 5, compute_rgb_color(part));
    break;
  case PART_KIND_SEVEN_SEGMENT:
    canvas_fill_rect(bounds.left, bounds.top, width, height, DARK_BODY_COLOR);
    draw_seven_segments(part, center_x, center_y);
    break;
  case PART_KIND_TACT_SWITCH:
    canvas_fill_rect(bounds.left, bounds.top, width, height, DARK_BODY_COLOR);
    canvas_fill_circle(
      center_x,
      center_y,
      3,
      part->pressed_frames > 0 ? ACTIVE_COLOR : METAL_COLOR
    );
    break;
  case PART_KIND_SLIDE_SWITCH: {
    int knob_x = part->slide_position == 0 ? bounds.left + 1 : center_x;

    canvas_fill_rect(bounds.left, bounds.top, width, height, BLUE_BODY_COLOR);
    canvas_fill_rect(
      knob_x,
      bounds.top + 1,
      width / 2,
      height - 2,
      METAL_COLOR
    );
    break;
  }
  case PART_KIND_NPN:
    canvas_fill_rect(bounds.left, bounds.top, width, height, DARK_BODY_COLOR);
    canvas_line(bounds.left, bounds.top, bounds.right, bounds.top, METAL_COLOR);
    break;
  case PART_KIND_VOLUME:
    canvas_fill_rect(bounds.left, bounds.top, width, height, BLUE_BODY_COLOR);
    draw_knob(part, center_x, center_y);
    break;
  case PART_KIND_RELAY:
    canvas_fill_rect(bounds.left, bounds.top, width, height, BLUE_BODY_COLOR);
    canvas_fill_rect(
      center_x - 3,
      center_y - 3,
      7,
      7,
      part->energized ? ACTIVE_COLOR : METAL_COLOR
    );
    break;
  default:
    break;
  }

  draw_module_pins(part);
}
