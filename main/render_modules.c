#include "breadboard.h"

#include "canvas.h"
#include "theme.h"

#define DARK_BODY_COLOR 0x202020
#define BLUE_BODY_COLOR 0x2050B0
#define METAL_COLOR 0xB0B0B0
#define ACTIVE_COLOR 0xFFD020
#define BURNED_COLOR 0x282828
#define SEGMENT_UNLIT_COLOR 0x401010
#define SEGMENT_LIT_COLOR 0xFF3020
#define RGB_CHANNEL_COUNT 3
#define SEVEN_SEGMENT_COUNT 8
#define PINO_BODY_COLOR 0x1E6B30
#define PINO_LED_LIT_COLOR 0x30FF40
#define PINO_LED_UNLIT_COLOR 0x204020
#define PINO_USB_WIDTH 6
#define PINO_USB_HEIGHT 10
#define PINO_LED_SIZE 3
#define PINO_PAD_COLOR 0xC8A040
#define PINO_TRACE_COLOR 0x38A050
#define PINO_SILK_COLOR 0xE8E8E8
#define PINO_BUTTON_COLOR 0xB8B8B8
#define PINO_CHIP_SIZE 15

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
draw_pino_pads(const Part *part, int center_y) {
  for (int i = 0; i < part->terminal_count; i++) {
    int x = compute_hole_x(part->terminal_hole_indices[i]);
    int y = compute_hole_y(part->terminal_hole_indices[i]);
    int stub_y = y < center_y ? y + 5 : y - 5;

    canvas_line(x, y, x, stub_y, PINO_TRACE_COLOR);
    canvas_fill_rect(x - 2, y - 2, 5, 5, PINO_PAD_COLOR);
  }
}

static void
draw_pino_chip_traces(int chip_x, int chip_y, int top, int bottom) {
  int middle_x = chip_x + PINO_CHIP_SIZE / 2;

  for (int i = -1; i <= 2; i++) {
    int pad_x = middle_x - 11 + i * 7;
    int pin_x = middle_x + i * 4 - 2;

    canvas_line(pad_x, top, pad_x, chip_y - 5, PINO_TRACE_COLOR);
    canvas_line(pad_x, chip_y - 5, pin_x, chip_y - 1, PINO_TRACE_COLOR);
    canvas_line(
      pad_x,
      bottom,
      pad_x,
      chip_y + PINO_CHIP_SIZE + 4,
      PINO_TRACE_COLOR
    );
    canvas_line(
      pad_x,
      chip_y + PINO_CHIP_SIZE + 4,
      pin_x,
      chip_y + PINO_CHIP_SIZE,
      PINO_TRACE_COLOR
    );
  }
}

static void
draw_pino_via_trace(int x, int y, int step_x, int step_y) {
  int via_x = x + step_x * 5;
  int via_y = y + step_y * 5;

  canvas_line(x, y, via_x, via_y, PINO_TRACE_COLOR);
  canvas_fill_rect(via_x - 1, via_y - 1, 3, 3, PINO_TRACE_COLOR);
  canvas_pixel(via_x, via_y, DARK_BODY_COLOR);
}

static void
draw_pino_via_traces(const Bounds *bounds, int first_column, int step_x) {
  for (int i = 0; i < 4; i++) {
    int x = bounds->left + 3 + (first_column + i) * 7;

    draw_pino_via_trace(x, bounds->top + 8, step_x, 1);
    draw_pino_via_trace(x, bounds->bottom - 8, step_x, -1);
  }
}

static void
draw_pino_chip(int x, int y) {
  for (int i = 1; i < PINO_CHIP_SIZE; i += 2) {
    canvas_pixel(x + i, y - 1, METAL_COLOR);
    canvas_pixel(x + i, y + PINO_CHIP_SIZE, METAL_COLOR);
    canvas_pixel(x - 1, y + i, METAL_COLOR);
    canvas_pixel(x + PINO_CHIP_SIZE, y + i, METAL_COLOR);
  }

  canvas_fill_rect(x, y, PINO_CHIP_SIZE, PINO_CHIP_SIZE, DARK_BODY_COLOR);
  canvas_pixel(x + 2, y + 2, METAL_COLOR);
}

static void
draw_pino_flash(int x, int center_y) {
  for (int i = 1; i < 8; i += 2) {
    canvas_pixel(x + i, center_y - 4, METAL_COLOR);
    canvas_pixel(x + i, center_y + 4, METAL_COLOR);
  }

  canvas_fill_rect(x, center_y - 3, 8, 7, DARK_BODY_COLOR);
}

static void
draw_pino(const Part *part, const Bounds *bounds, int center_y) {
  int width = bounds->right - bounds->left + 1;
  int height = bounds->bottom - bounds->top + 1;
  int chip_x = bounds->left + width / 2 - PINO_CHIP_SIZE / 2;
  int chip_y = center_y - PINO_CHIP_SIZE / 2;
  int flash_x = chip_x - 22;

  canvas_fill_rect(bounds->left, bounds->top, width, height, PINO_BODY_COLOR);
  draw_pino_pads(part, center_y);
  draw_pino_chip_traces(chip_x, chip_y, bounds->top + 8, bounds->bottom - 8);
  draw_pino_via_traces(bounds, 2, 1);
  draw_pino_via_traces(bounds, 13, -1);

  for (int i = -2; i <= 2; i += 2)
    canvas_line(
      flash_x + 8,
      center_y + i,
      chip_x - 2,
      center_y + i,
      PINO_TRACE_COLOR
    );

  draw_pino_chip(chip_x, chip_y);
  draw_pino_flash(flash_x, center_y);
  canvas_fill_rect(
    chip_x + PINO_CHIP_SIZE + 5,
    center_y - 3,
    3,
    7,
    METAL_COLOR
  );
  canvas_fill_rect(bounds->left + 20, center_y + 2, 7, 7, PINO_BUTTON_COLOR);
  canvas_fill_circle(bounds->left + 23, center_y + 5, 2, PINO_SILK_COLOR);

  for (int i = -6; i <= 6; i += 6)
    canvas_fill_rect(bounds->right - 6, center_y + i - 1, 3, 3, PINO_PAD_COLOR);

  canvas_fill_rect(
    bounds->left - PINO_USB_WIDTH / 2,
    center_y - PINO_USB_HEIGHT / 2,
    PINO_USB_WIDTH,
    PINO_USB_HEIGHT,
    METAL_COLOR
  );
  canvas_fill_rect(
    bounds->left + PINO_USB_WIDTH,
    center_y - PINO_USB_HEIGHT / 2 - PINO_LED_SIZE,
    PINO_LED_SIZE,
    PINO_LED_SIZE,
    is_pino_led_lit() ? PINO_LED_LIT_COLOR : PINO_LED_UNLIT_COLOR
  );

  const char *label = find_part_name(PART_KIND_PINO);
  int label_left = chip_x + PINO_CHIP_SIZE + 12;
  int label_width = bounds->right - 8 - label_left;

  canvas_text(
    label_left + (label_width - canvas_text_width(label)) / 2,
    center_y - CANVAS_FONT_HEIGHT / 2,
    label,
    PINO_SILK_COLOR
  );
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
  case PART_KIND_PINO:
    draw_pino(part, &bounds, center_y);
    break;
  default:
    break;
  }

  draw_module_pins(part);
}
