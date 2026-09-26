#include "game.h"

#include "canvas.h"
#include "keyboard.h"
#include "page.h"
#include "theme.h"
#include "widget.h"

#include <stdio.h>

#define LOGO_X 22
#define LOGO_Y 8
#define LOGO_SCALE 5
#define LOGO_GLYPH_HEIGHT 7
#define LOGO_TOP_ROW_COUNT 4
#define LOGO_GLYPH_COUNT 4
#define LOGO_TOP_COLOR 0xF4E8D0
#define LOGO_BOTTOM_COLOR 0xF0501A
#define LOGO_EDGE_COLOR 0x8A8A8A
#define LOGO_OUTLINE_COLOR 0x000000
#define SPARK_COLOR 0xF8C020
#define ILLUSTRATION_X 22
#define ILLUSTRATION_Y 64
#define ILLUSTRATION_WIDTH 100
#define ILLUSTRATION_HEIGHT 62
#define ILLUSTRATION_EDGE_HEIGHT 4
#define ILLUSTRATION_COLUMN_COUNT 19
#define ILLUSTRATION_PITCH 5
#define ILLUSTRATION_HOLE_SIZE 2
#define ILLUSTRATION_ROW_PITCH 4
#define ILLUSTRATION_RAIL_GROUP_SIZE 6
#define ILLUSTRATION_GROOVE_Y 30
#define ILLUSTRATION_GROOVE_HEIGHT 2
#define ILLUSTRATION_TOP_RED_LINE_Y 2
#define ILLUSTRATION_TOP_PLUS_Y 5
#define ILLUSTRATION_TOP_MINUS_Y 9
#define ILLUSTRATION_TOP_BLUE_LINE_Y 13
#define ILLUSTRATION_TOP_ROW_Y 17
#define ILLUSTRATION_GROOVE_ROW_Y 25
#define ILLUSTRATION_BELOW_GROOVE_ROW_Y 35
#define ILLUSTRATION_BOTTOM_ROW_Y 43
#define ILLUSTRATION_BOTTOM_BLUE_LINE_Y 48
#define ILLUSTRATION_BOTTOM_MINUS_Y 51
#define ILLUSTRATION_BOTTOM_PLUS_Y 55
#define ILLUSTRATION_BOTTOM_RED_LINE_Y 59
#define ILLUSTRATION_RAIL_WIRE_HEIGHT 8
#define ILLUSTRATION_GROUND_WIRE_HEIGHT 5
#define ILLUSTRATION_BOARD_COLOR 0xE8E2D0
#define ILLUSTRATION_EDGE_COLOR 0xB8B0A0
#define ILLUSTRATION_EDGE_SHADOW_COLOR 0x908878
#define ILLUSTRATION_GROOVE_COLOR 0xC4BCAA
#define ILLUSTRATION_HOLE_COLOR 0x4A4A4A
#define ILLUSTRATION_RED_COLOR 0xD03030
#define ILLUSTRATION_BLUE_COLOR 0x3050D0
#define ILLUSTRATION_LED_COLOR 0xF83028
#define ILLUSTRATION_LED_SHADE_COLOR 0xB81818
#define ILLUSTRATION_LED_SHINE_COLOR 0xFFE0C8
#define ILLUSTRATION_LEAD_COLOR 0x9A9A9A
#define ILLUSTRATION_RESISTOR_COLOR 0xD8B070
#define ILLUSTRATION_RESISTOR_SHADE_COLOR 0xB08850
#define ILLUSTRATION_BAND_COLOR 0x803010
#define ILLUSTRATION_GOLD_BAND_COLOR 0xC09030
#define ILLUSTRATION_CHIP_TOP_COLOR 0x303030
#define ILLUSTRATION_CHIP_FRONT_COLOR 0x181818
#define ILLUSTRATION_PIN_MARK_COLOR 0x686868
#define ILLUSTRATION_LED_ANODE_COLUMN 9
#define ILLUSTRATION_LED_CATHODE_COLUMN 10
#define ILLUSTRATION_LED_TOP_Y 50
#define ILLUSTRATION_RESISTOR_COLUMN 5
#define ILLUSTRATION_RESISTOR_HEIGHT 3
#define ILLUSTRATION_CHIP_COLUMN 14
#define ILLUSTRATION_CHIP_PIN_COUNT 4
#define ILLUSTRATION_RAIL_LINK_COLUMN 17
#define ILLUSTRATION_BATTERY_PLUS_COLUMN 0
#define ILLUSTRATION_BATTERY_MINUS_COLUMN 1
#define ILLUSTRATION_BATTERY_CENTER_Y 59
#define ILLUSTRATION_BATTERY_RADIUS 6
#define ILLUSTRATION_BATTERY_EDGE_COLOR 0x888888
#define ILLUSTRATION_BATTERY_FACE_COLOR 0xD0D0D0
#define ILLUSTRATION_BATTERY_SHINE_COLOR 0xF4F4F4
#define ILLUSTRATION_BATTERY_MARK_COLOR 0x606060
#define ILLUSTRATION_HOLDER_COLOR 0x202020
#define MENU_X 150
#define MENU_Y 40
#define MENU_ROW_HEIGHT 16
#define MENU_MARKER_X 142
#define MENU_TEXT_COLOR 0xF4E8D0
#define HISCORE_Y 120

typedef enum {
  TITLE_ITEM_SIMULATOR,
  TITLE_ITEM_GAME,
  TITLE_ITEM_HOW_TO_PLAY,
  TITLE_ITEM_COUNT
} TitleItem;

typedef struct {
  int width;
  const char *rows[LOGO_GLYPH_HEIGHT];
} LogoGlyph;

static const LogoGlyph LOGO_GLYPHS[LOGO_GLYPH_COUNT] = {
  {5, {"11110", "11011", "11011", "11110", "11000", "11000", "11000"}},
  {4, {"1111", "0110", "0110", "0110", "0110", "0110", "1111"}},
  {6, {"110011", "111011", "111111", "110111", "110011", "110011", "110011"}},
  {2, {"11", "11", "11", "11", "11", "00", "11"}},
};

static const char *const TITLE_ITEM_LABELS[TITLE_ITEM_COUNT] = {
  "SIMULATOR",
  "GAME",
  "HOW TO PLAY",
};

static const PageLine HOW_TO_PLAY_LINES[] = {
  {PAGE_LINE_HEADING, NULL, "GOAL"},
  {PAGE_LINE_TEXT, NULL, "Parts are dealt to you one by one."},
  {PAGE_LINE_TEXT, NULL, "Put each one on the breadboard."},
  {PAGE_LINE_TEXT, NULL, "Light an LED, sound the buzzer or"},
  {PAGE_LINE_TEXT, NULL, "spin the motor, and every part on"},
  {PAGE_LINE_TEXT, NULL, "that circuit is cleared. Clearing"},
  {PAGE_LINE_TEXT, NULL, "n parts scores n x n points. The"},
  {PAGE_LINE_TEXT, NULL, "battery stays."},
  {PAGE_LINE_TEXT, NULL, "Place 40 parts to clear the game."},
  {PAGE_LINE_BLANK, NULL, NULL},
  {PAGE_LINE_HEADING, NULL, "THE BOARD"},
  {PAGE_LINE_TEXT, NULL, "A 5V battery sits on the top-left"},
  {PAGE_LINE_TEXT, NULL, "rails. Red line is +, blue is -."},
  {PAGE_LINE_TEXT, NULL, "The top and bottom rails are not"},
  {PAGE_LINE_TEXT, NULL, "joined: wire them to use both."},
  {PAGE_LINE_TEXT, NULL, "Holes a-e in a column are joined,"},
  {PAGE_LINE_TEXT, NULL, "and so are f-j. The groove in the"},
  {PAGE_LINE_TEXT, NULL, "middle keeps the two apart."},
  {PAGE_LINE_TEXT, NULL, "Any part may use the rails."},
  {PAGE_LINE_BLANK, NULL, NULL},
  {PAGE_LINE_HEADING, NULL, "PLACING"},
  {PAGE_LINE_TEXT, NULL, "The header shows the part and the"},
  {PAGE_LINE_TEXT, NULL, "pin to set next, e.g. anode+."},
  {PAGE_LINE_TEXT, NULL, "Lead parts take two pins: move and"},
  {PAGE_LINE_TEXT, NULL, "press Enter for each. Modules drop"},
  {PAGE_LINE_TEXT, NULL, "at the cursor with one Enter."},
  {PAGE_LINE_TEXT, NULL, "Jumper wires come 3 times in 10."},
  {PAGE_LINE_TEXT, NULL, "Single LEDs are always red, and no"},
  {PAGE_LINE_TEXT, NULL, "battery is ever dealt."},
  {PAGE_LINE_BLANK, NULL, NULL},
  {PAGE_LINE_HEADING, NULL, "CLEARING"},
  {PAGE_LINE_TEXT, NULL, "When an output runs, the parts on"},
  {PAGE_LINE_TEXT, NULL, "its current path blink yellow and"},
  {PAGE_LINE_TEXT, NULL, "the header says CLEAR!. Then they"},
  {PAGE_LINE_TEXT, NULL, "are removed with their wires."},
  {PAGE_LINE_BLANK, NULL, NULL},
  {PAGE_LINE_HEADING, NULL, "TIPS"},
  {PAGE_LINE_TEXT, NULL, "An LED needs a resistor in series."},
  {PAGE_LINE_TEXT, NULL, "A resistor starts at 220 ohm."},
  {PAGE_LINE_TEXT, NULL, "10 ohm burns a red LED; 10k ohm or"},
  {PAGE_LINE_TEXT, NULL, "more keeps it dark."},
  {PAGE_LINE_TEXT, NULL, "LEDs and diodes have a direction:"},
  {PAGE_LINE_TEXT, NULL, "anode+ goes toward battery +."},
  {PAGE_LINE_TEXT, NULL, "space on a tact switch presses it,"},
  {PAGE_LINE_TEXT, NULL, "on a slide switch flips it."},
  {PAGE_LINE_TEXT, NULL, "+ / - on a resistor, volume or CdS"},
  {PAGE_LINE_TEXT, NULL, "changes its value."},
  {PAGE_LINE_TEXT, NULL, "` (ESC) puts the part down. Then"},
  {PAGE_LINE_TEXT, NULL, "the header shows the part under"},
  {PAGE_LINE_TEXT, NULL, "the cursor, i shows what it is and"},
  {PAGE_LINE_TEXT, NULL, "? shows the keys. Enter picks the"},
  {PAGE_LINE_TEXT, NULL, "part up again."},
  {PAGE_LINE_BLANK, NULL, NULL},
  {PAGE_LINE_HEADING, NULL, "GAME OVER"},
  {PAGE_LINE_TEXT, NULL, "- an LED or 7-seg burns out"},
  {PAGE_LINE_TEXT, NULL, "- the battery is shorted"},
  {PAGE_LINE_TEXT, NULL, "- the next part fits nowhere"},
  {PAGE_LINE_TEXT, NULL, "- more than 15 parts are placed"},
  {PAGE_LINE_TEXT, NULL, "The header shows S score, P n/15"},
  {PAGE_LINE_TEXT, NULL, "and L, the parts left to place."},
#if !defined(PIN_BOARD_WASM)
  {PAGE_LINE_TEXT, NULL, "A new best is saved as HI-SCORE."},
#endif
  {PAGE_LINE_BLANK, NULL, NULL},
  {PAGE_LINE_HEADING, NULL, "KEYS"},
  {PAGE_LINE_KEY, "hjkl", "move the cursor"},
  {PAGE_LINE_KEY, "arrows", "also move the cursor"},
  {PAGE_LINE_KEY, "Enter", "set the next pin"},
  {PAGE_LINE_KEY, "space", "press / flip a switch"},
  {PAGE_LINE_KEY, "+ -", "change a value"},
  {PAGE_LINE_KEY, "` (ESC)", "redo the first pin / put down"},
  {PAGE_LINE_KEY, "Enter", "pick up (part put down)"},
  {PAGE_LINE_KEY, "i", "part info (part put down)"},
  {PAGE_LINE_KEY, "?", "help (part put down)"},
  {PAGE_LINE_KEY, "q", "quit to the title"},
};

#define HOW_TO_PLAY_LINE_COUNT COUNT_PAGE_LINES(HOW_TO_PLAY_LINES)

// Draws every filled cell grown by `grow` pixels, so outlines stack.
static void
draw_logo_layer(int grow, bool filled) {
  int glyph_x = LOGO_X;

  for (int i = 0; i < LOGO_GLYPH_COUNT; i++) {
    const LogoGlyph *glyph = &LOGO_GLYPHS[i];

    for (int row = 0; row < LOGO_GLYPH_HEIGHT; row++) {
      for (int column = 0; column < glyph->width; column++) {
        if (glyph->rows[row][column] != '1')
          continue;

        uint32_t color = LOGO_EDGE_COLOR;

        if (filled)
          color = row < LOGO_TOP_ROW_COUNT ? LOGO_TOP_COLOR : LOGO_BOTTOM_COLOR;
        else if (grow == 1)
          color = LOGO_OUTLINE_COLOR;

        canvas_fill_rect(
          glyph_x + column * LOGO_SCALE - grow,
          LOGO_Y + row * LOGO_SCALE - grow,
          LOGO_SCALE + grow * 2,
          LOGO_SCALE + grow * 2,
          color
        );
      }
    }

    glyph_x += (glyph->width + 1) * LOGO_SCALE;
  }
}

static void
draw_thick_line(int x0, int y0, int x1, int y1, uint32_t color) {
  canvas_line(x0, y0, x1, y1, color);
  canvas_line(x0 + 1, y0, x1 + 1, y1, color);
}

static void
draw_sparks(void) {
  draw_thick_line(8, 10, 13, 14, SPARK_COLOR);
  draw_thick_line(6, 24, 12, 25, SPARK_COLOR);
  draw_thick_line(8, 39, 13, 35, SPARK_COLOR);
  draw_thick_line(127, 14, 132, 10, SPARK_COLOR);
  draw_thick_line(128, 25, 134, 24, SPARK_COLOR);
  draw_thick_line(127, 35, 132, 39, SPARK_COLOR);
}

static void
draw_logo(void) {
  draw_logo_layer(2, false);
  draw_logo_layer(1, false);
  draw_logo_layer(0, true);
  draw_sparks();
}

static int
compute_illustration_hole_x(int column) {
  return ILLUSTRATION_X + ILLUSTRATION_PITCH + column * ILLUSTRATION_PITCH;
}

static int
compute_illustration_hole_y(int row_y) {
  return ILLUSTRATION_Y + row_y;
}

static void
draw_illustration_holes(int row_y, bool rail) {
  for (int column = 0; column < ILLUSTRATION_COLUMN_COUNT; column++) {
    if (rail && column % ILLUSTRATION_RAIL_GROUP_SIZE == 0 && column > 0 &&
        column < ILLUSTRATION_COLUMN_COUNT - 1)
      continue;

    canvas_fill_rect(
      compute_illustration_hole_x(column),
      compute_illustration_hole_y(row_y),
      ILLUSTRATION_HOLE_SIZE,
      ILLUSTRATION_HOLE_SIZE,
      ILLUSTRATION_HOLE_COLOR
    );
  }
}

static void
draw_illustration_rail_line(int row_y, uint32_t color) {
  canvas_line(
    ILLUSTRATION_X + 3,
    compute_illustration_hole_y(row_y),
    ILLUSTRATION_X + ILLUSTRATION_WIDTH - 4,
    compute_illustration_hole_y(row_y),
    color
  );
}

static void
draw_illustration_board(void) {
  canvas_fill_rect(
    ILLUSTRATION_X,
    ILLUSTRATION_Y + ILLUSTRATION_HEIGHT,
    ILLUSTRATION_WIDTH,
    ILLUSTRATION_EDGE_HEIGHT,
    ILLUSTRATION_EDGE_COLOR
  );
  canvas_line(
    ILLUSTRATION_X,
    ILLUSTRATION_Y + ILLUSTRATION_HEIGHT + ILLUSTRATION_EDGE_HEIGHT - 1,
    ILLUSTRATION_X + ILLUSTRATION_WIDTH - 1,
    ILLUSTRATION_Y + ILLUSTRATION_HEIGHT + ILLUSTRATION_EDGE_HEIGHT - 1,
    ILLUSTRATION_EDGE_SHADOW_COLOR
  );
  canvas_fill_rect(
    ILLUSTRATION_X,
    ILLUSTRATION_Y,
    ILLUSTRATION_WIDTH,
    ILLUSTRATION_HEIGHT,
    ILLUSTRATION_BOARD_COLOR
  );
  canvas_fill_rect(
    ILLUSTRATION_X,
    compute_illustration_hole_y(ILLUSTRATION_GROOVE_Y),
    ILLUSTRATION_WIDTH,
    ILLUSTRATION_GROOVE_HEIGHT,
    ILLUSTRATION_GROOVE_COLOR
  );
  draw_illustration_rail_line(
    ILLUSTRATION_TOP_RED_LINE_Y,
    ILLUSTRATION_RED_COLOR
  );
  draw_illustration_rail_line(
    ILLUSTRATION_TOP_BLUE_LINE_Y,
    ILLUSTRATION_BLUE_COLOR
  );
  draw_illustration_rail_line(
    ILLUSTRATION_BOTTOM_BLUE_LINE_Y,
    ILLUSTRATION_BLUE_COLOR
  );
  draw_illustration_rail_line(
    ILLUSTRATION_BOTTOM_RED_LINE_Y,
    ILLUSTRATION_RED_COLOR
  );

  draw_illustration_holes(ILLUSTRATION_TOP_PLUS_Y, true);
  draw_illustration_holes(ILLUSTRATION_TOP_MINUS_Y, true);
  draw_illustration_holes(ILLUSTRATION_BOTTOM_MINUS_Y, true);
  draw_illustration_holes(ILLUSTRATION_BOTTOM_PLUS_Y, true);

  for (int y = ILLUSTRATION_TOP_ROW_Y; y <= ILLUSTRATION_GROOVE_ROW_Y;
       y += ILLUSTRATION_ROW_PITCH)
    draw_illustration_holes(y, false);

  for (int y = ILLUSTRATION_BELOW_GROOVE_ROW_Y; y <= ILLUSTRATION_BOTTOM_ROW_Y;
       y += ILLUSTRATION_ROW_PITCH)
    draw_illustration_holes(y, false);
}

// Line drawn with a 2x2 pen.
static void
draw_wide_line(int x0, int y0, int x1, int y1, uint32_t color) {
  for (int dy = 0; dy < 2; dy++)
    for (int dx = 0; dx < 2; dx++)
      canvas_line(x0 + dx, y0 + dy, x1 + dx, y1 + dy, color);
}

// A jumper wire bent up out of the back hole, across, and down into the
// front hole. Both ends stop just above their holes so the holes stay visible.
static void
draw_illustration_wire(
  int back_column,
  int back_row_y,
  int front_column,
  int front_row_y,
  int height,
  uint32_t color
) {
  int back_x = compute_illustration_hole_x(back_column);
  int back_y = compute_illustration_hole_y(back_row_y);
  int front_x = compute_illustration_hole_x(front_column);
  int front_y = compute_illustration_hole_y(front_row_y);
  int top = back_y - height;
  int left = back_x < front_x ? back_x : front_x;
  int right = back_x < front_x ? front_x : back_x;

  canvas_fill_rect(back_x, top + 1, 2, back_y - top - 2, color);
  canvas_fill_rect(left + 1, top, right - left, 2, color);
  canvas_fill_rect(front_x, top + 1, 2, front_y - top - 2, color);
  canvas_fill_rect(back_x, back_y - 1, 2, 1, ILLUSTRATION_LEAD_COLOR);
  canvas_fill_rect(front_x, front_y - 1, 2, 1, ILLUSTRATION_LEAD_COLOR);
}

// A lead from `top` down to just above the hole.
static void
draw_illustration_lead(int column, int row_y, int top) {
  int x = compute_illustration_hole_x(column);

  canvas_line(
    x,
    top,
    x,
    compute_illustration_hole_y(row_y) - 1,
    ILLUSTRATION_LEAD_COLOR
  );
}

// Lying along a row, standing on its two leads.
static void
draw_illustration_resistor(void) {
  int left = compute_illustration_hole_x(ILLUSTRATION_RESISTOR_COLUMN);
  int right = compute_illustration_hole_x(ILLUSTRATION_LED_ANODE_COLUMN);
  int y = compute_illustration_hole_y(ILLUSTRATION_GROOVE_ROW_Y) -
          ILLUSTRATION_RESISTOR_HEIGHT;
  int body_x = (left + right) / 2 - 5;

  draw_illustration_lead(
    ILLUSTRATION_RESISTOR_COLUMN,
    ILLUSTRATION_GROOVE_ROW_Y,
    y
  );
  draw_illustration_lead(
    ILLUSTRATION_LED_ANODE_COLUMN,
    ILLUSTRATION_GROOVE_ROW_Y,
    y
  );
  canvas_line(left, y, right, y, ILLUSTRATION_LEAD_COLOR);

  canvas_fill_rect(body_x, y - 2, 11, 5, ILLUSTRATION_RESISTOR_COLOR);
  canvas_fill_rect(body_x, y + 2, 11, 1, ILLUSTRATION_RESISTOR_SHADE_COLOR);
  canvas_fill_rect(body_x + 2, y - 2, 1, 5, ILLUSTRATION_BAND_COLOR);
  canvas_fill_rect(body_x + 4, y - 2, 1, 5, ILLUSTRATION_BAND_COLOR);
  canvas_fill_rect(body_x + 6, y - 2, 1, 5, ILLUSTRATION_BAND_COLOR);
  canvas_fill_rect(body_x + 8, y - 2, 1, 5, ILLUSTRATION_GOLD_BAND_COLOR);
}

// Standing in the top row, reaching up under the logo.
static void
draw_illustration_led(void) {
  int left = compute_illustration_hole_x(ILLUSTRATION_LED_ANODE_COLUMN) - 1;
  int top = ILLUSTRATION_LED_TOP_Y;
  int center = left + 4;

  draw_illustration_lead(
    ILLUSTRATION_LED_ANODE_COLUMN,
    ILLUSTRATION_TOP_ROW_Y,
    top + 10
  );
  draw_illustration_lead(
    ILLUSTRATION_LED_CATHODE_COLUMN,
    ILLUSTRATION_TOP_MINUS_Y,
    top + 10
  );

  canvas_fill_rect(left + 2, top, 4, 1, ILLUSTRATION_LED_COLOR);
  canvas_fill_rect(left + 1, top + 1, 6, 1, ILLUSTRATION_LED_COLOR);
  canvas_fill_rect(left, top + 2, 8, 6, ILLUSTRATION_LED_COLOR);
  canvas_fill_rect(left + 5, top + 2, 3, 6, ILLUSTRATION_LED_SHADE_COLOR);
  canvas_fill_rect(left - 1, top + 8, 10, 2, ILLUSTRATION_LED_SHADE_COLOR);
  canvas_fill_rect(left + 2, top + 1, 2, 1, ILLUSTRATION_LED_SHINE_COLOR);
  canvas_fill_rect(left + 1, top + 2, 1, 3, ILLUSTRATION_LED_SHINE_COLOR);

  draw_thick_line(center - 11, top - 1, center - 8, top + 1, SPARK_COLOR);
  draw_thick_line(center - 12, top + 5, center - 8, top + 5, SPARK_COLOR);
  draw_thick_line(center + 7, top + 1, center + 10, top - 1, SPARK_COLOR);
  draw_thick_line(center + 7, top + 5, center + 11, top + 5, SPARK_COLOR);
}

// An 8-pin DIP straddling the groove. Its back pins sit under the body.
static void
draw_illustration_chip(void) {
  int left = compute_illustration_hole_x(ILLUSTRATION_CHIP_COLUMN) - 2;
  int right = compute_illustration_hole_x(
                ILLUSTRATION_CHIP_COLUMN + ILLUSTRATION_CHIP_PIN_COUNT - 1
              ) +
              ILLUSTRATION_HOLE_SIZE + 2;
  int top = compute_illustration_hole_y(ILLUSTRATION_GROOVE_ROW_Y) - 3;
  int front = compute_illustration_hole_y(ILLUSTRATION_BELOW_GROOVE_ROW_Y) - 3;

  canvas_fill_rect(
    left,
    top,
    right - left,
    front - top,
    ILLUSTRATION_CHIP_TOP_COLOR
  );
  canvas_fill_rect(left, front, right - left, 2, ILLUSTRATION_CHIP_FRONT_COLOR);
  canvas_fill_rect(
    compute_illustration_hole_x(ILLUSTRATION_CHIP_COLUMN),
    front - 3,
    2,
    1,
    ILLUSTRATION_PIN_MARK_COLOR
  );

  for (int i = 0; i < ILLUSTRATION_CHIP_PIN_COUNT; i++)
    canvas_fill_rect(
      compute_illustration_hole_x(ILLUSTRATION_CHIP_COLUMN + i),
      front + 1,
      2,
      2,
      ILLUSTRATION_LEAD_COLOR
    );
}

// A coin cell standing in a holder whose pins go straight into the top rails.
static void
draw_illustration_battery(void) {
  int plus_x = compute_illustration_hole_x(ILLUSTRATION_BATTERY_PLUS_COLUMN);
  int minus_x = compute_illustration_hole_x(ILLUSTRATION_BATTERY_MINUS_COLUMN);
  int center_x = (plus_x + minus_x + 1) / 2;
  int center_y = ILLUSTRATION_BATTERY_CENTER_Y;
  int radius = ILLUSTRATION_BATTERY_RADIUS;
  int holder_top = center_y + 3;
  int holder_bottom = center_y + radius + 2;

  draw_illustration_lead(
    ILLUSTRATION_BATTERY_PLUS_COLUMN,
    ILLUSTRATION_TOP_PLUS_Y,
    holder_bottom
  );
  draw_illustration_lead(
    ILLUSTRATION_BATTERY_MINUS_COLUMN,
    ILLUSTRATION_TOP_MINUS_Y,
    holder_bottom
  );

  canvas_fill_circle(
    center_x,
    center_y,
    radius,
    ILLUSTRATION_BATTERY_EDGE_COLOR
  );
  canvas_fill_circle(
    center_x,
    center_y,
    radius - 1,
    ILLUSTRATION_BATTERY_FACE_COLOR
  );
  canvas_fill_rect(
    center_x - 3,
    center_y - 4,
    2,
    1,
    ILLUSTRATION_BATTERY_SHINE_COLOR
  );
  canvas_fill_rect(
    center_x - 4,
    center_y - 3,
    1,
    2,
    ILLUSTRATION_BATTERY_SHINE_COLOR
  );
  canvas_line(
    center_x - 1,
    center_y - 1,
    center_x + 1,
    center_y - 1,
    ILLUSTRATION_BATTERY_MARK_COLOR
  );
  canvas_line(
    center_x,
    center_y - 2,
    center_x,
    center_y,
    ILLUSTRATION_BATTERY_MARK_COLOR
  );

  canvas_fill_rect(
    center_x - radius - 1,
    holder_top,
    radius * 2 + 3,
    holder_bottom - holder_top,
    ILLUSTRATION_HOLDER_COLOR
  );
}

// Battery -> red rail -> wire -> resistor -> LED -> blue rail, so the LED is
// lit. The chip is powered from the rails too, its ground through the bottom
// blue rail linked to the top one.
static void
draw_illustration_parts(void) {
  draw_illustration_battery();
  draw_illustration_led();
  draw_illustration_wire(
    3,
    ILLUSTRATION_TOP_PLUS_Y,
    ILLUSTRATION_RESISTOR_COLUMN,
    ILLUSTRATION_TOP_ROW_Y,
    ILLUSTRATION_RAIL_WIRE_HEIGHT,
    ILLUSTRATION_RED_COLOR
  );
  draw_illustration_wire(
    16,
    ILLUSTRATION_TOP_PLUS_Y,
    ILLUSTRATION_CHIP_COLUMN,
    ILLUSTRATION_TOP_ROW_Y,
    ILLUSTRATION_RAIL_WIRE_HEIGHT,
    ILLUSTRATION_RED_COLOR
  );
  draw_illustration_resistor();
  draw_illustration_chip();
  draw_illustration_wire(
    ILLUSTRATION_CHIP_COLUMN,
    ILLUSTRATION_BOTTOM_ROW_Y,
    15,
    ILLUSTRATION_BOTTOM_MINUS_Y,
    ILLUSTRATION_GROUND_WIRE_HEIGHT,
    ILLUSTRATION_BLUE_COLOR
  );
  draw_illustration_wire(
    ILLUSTRATION_RAIL_LINK_COLUMN,
    ILLUSTRATION_TOP_MINUS_Y,
    ILLUSTRATION_RAIL_LINK_COLUMN + 1,
    ILLUSTRATION_BOTTOM_MINUS_Y,
    ILLUSTRATION_RAIL_WIRE_HEIGHT,
    ILLUSTRATION_BLUE_COLOR
  );
}

static void
draw_menu_marker(int y) {
  for (int i = 0; i < 4; i++)
    canvas_line(
      MENU_MARKER_X + i,
      y + i,
      MENU_MARKER_X + i,
      y + 8 - i,
      MENU_TEXT_COLOR
    );
}

static void
draw_title_menu(int selected_index) {
  for (int i = 0; i < TITLE_ITEM_COUNT; i++) {
    int y = MENU_Y + i * MENU_ROW_HEIGHT;

    if (i == selected_index)
      draw_menu_marker(y + 2);

    canvas_text(MENU_X, y, TITLE_ITEM_LABELS[i], MENU_TEXT_COLOR);
  }
}

// The browser build keeps no hiscore, so it shows none.
#if !defined(PIN_BOARD_WASM)
static void
draw_hiscore(int hiscore) {
  char text[TEXT_SIZE];

  snprintf(text, sizeof text, "HI-SCORE %d", hiscore);
  canvas_text(MENU_X, HISCORE_Y, text, MENU_TEXT_COLOR);
}
#endif

static void
draw_title_screen(int selected_index, int hiscore) {
  while (canvas_begin_band()) {
    canvas_fill(THEME_BACKGROUND_COLOR);
    draw_illustration_board();
    draw_illustration_parts();
    draw_logo();
    draw_title_menu(selected_index);
#if !defined(PIN_BOARD_WASM)
    draw_hiscore(hiscore);
#endif
    canvas_push_band();
  }
}

TitleChoice
run_title_screen(void) {
  int selected_index = TITLE_ITEM_SIMULATOR;
  int hiscore = load_hiscore();

  for (;;) {
    draw_title_screen(selected_index, hiscore);

    int key = decode_key(keyboard_wait_key());

    if (key == KEY_UP && selected_index > 0)
      selected_index--;

    if (key == KEY_DOWN && selected_index < TITLE_ITEM_COUNT - 1)
      selected_index++;

    if (key == 'q' && widget_run_confirm("Quit?"))
      return TITLE_CHOICE_QUIT;

    if (key != KEY_ENTER && key != ' ')
      continue;

    switch (selected_index) {
    case TITLE_ITEM_SIMULATOR:
      return TITLE_CHOICE_SIMULATOR;
    case TITLE_ITEM_GAME:
      return TITLE_CHOICE_GAME;
    default:
      show_page("HOW TO PLAY", HOW_TO_PLAY_LINES, HOW_TO_PLAY_LINE_COUNT);
      break;
    }
  }
}
