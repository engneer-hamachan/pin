#include "widget.h"

#include "canvas.h"
#include "keyboard.h"
#include "theme.h"

#include <string.h>

#define CLIPPED_TEXT_SIZE 128
#define TITLED_PANEL_TITLE_HEIGHT 15
#define CONFIRM_PANEL_HEIGHT 36
#define CONFIRM_PANEL_BOTTOM_MARGIN 50
#define CONFIRM_HINT "[y] Yes   [n] No_"
#define MENU_MINIMUM_WIDTH 96
#define MENU_SIDE_MARGIN 24
#define MENU_VERTICAL_MARGIN 34

static int
clamp(int value, int minimum, int maximum) {
  if (value < minimum)
    return minimum;

  if (value > maximum)
    return maximum;

  return value;
}

static int
compute_centered_text_y(int frame_y, int frame_height) {
  return frame_y + (frame_height - CANVAS_FONT_HEIGHT) / 2;
}

static void
clip_text(const char *text, int width, char *clipped, int clipped_size) {
  int length = (int)strlen(text);

  if (length >= clipped_size)
    length = clipped_size - 1;

  memcpy(clipped, text, length);
  clipped[length] = 0;

  if (canvas_text_width(clipped) <= width)
    return;

  while (length > 0) {
    length--;

    if (length + 1 >= clipped_size)
      continue;

    clipped[length] = '>';
    clipped[length + 1] = 0;

    if (canvas_text_width(clipped) <= width)
      return;
  }

  clipped[0] = 0;
}

void
widget_draw_header(const char *title, const char *right_text) {
  int text_y = compute_centered_text_y(0, WIDGET_HEADER_HEIGHT);
  char clipped[CLIPPED_TEXT_SIZE];

  canvas_rect(0, 0, CANVAS_WIDTH - 1, WIDGET_HEADER_HEIGHT, THEME_BORDER_COLOR);

  clip_text(title, CANVAS_WIDTH - 10, clipped, sizeof clipped);
  canvas_text(5, text_y, clipped, THEME_EMPHASIS_COLOR);

  if (right_text[0]) {
    clip_text(right_text, CANVAS_WIDTH / 2, clipped, sizeof clipped);
    canvas_text(
      CANVAS_WIDTH - 5 - canvas_text_width(clipped),
      text_y,
      clipped,
      THEME_TEXT_COLOR
    );
  }
}

void
widget_draw_panel(int x, int y, int width, int height) {
  if (width <= 0 || height <= 0)
    return;

  canvas_fill_rect(x, y, width, height, THEME_BOX_COLOR);
  canvas_rect(x, y, width, height, THEME_BORDER_COLOR);
}

static void
draw_titled_panel(int x, int y, int width, int height, const char *title) {
  char clipped[CLIPPED_TEXT_SIZE];

  widget_draw_panel(x, y, width, height);
  clip_text(title, width - 8, clipped, sizeof clipped);
  canvas_text(
    x + 4,
    compute_centered_text_y(y, TITLED_PANEL_TITLE_HEIGHT),
    clipped,
    THEME_EMPHASIS_COLOR
  );
  canvas_line(
    x,
    y + TITLED_PANEL_TITLE_HEIGHT - 1,
    x + width - 1,
    y + TITLED_PANEL_TITLE_HEIGHT - 1,
    THEME_BORDER_COLOR
  );
}

static void
draw_scrollbar(int x, int y, int height, int top, int visible, int total) {
  if (total <= visible || visible <= 0 || height < 3)
    return;

  top = clamp(top, 0, total - visible);

  int thumb_height = height * visible / total;

  if (thumb_height < 6)
    thumb_height = 6;

  if (thumb_height > height)
    thumb_height = height;

  int thumb_y = y + height * top / total;

  if (thumb_y + thumb_height > y + height)
    thumb_y = y + height - thumb_height;

  canvas_fill_rect(x, y, 3, height, THEME_BOX_COLOR);
  canvas_fill_rect(x, thumb_y, 3, thumb_height, THEME_BORDER_COLOR);
}

static int
compute_menu_width(
  const char *title,
  const char *const *items,
  int item_count
) {
  int width = canvas_text_width(title) + MENU_SIDE_MARGIN;

  for (int i = 0; i < item_count; i++) {
    int item_width = canvas_text_width(items[i]) + MENU_SIDE_MARGIN;

    if (item_width > width)
      width = item_width;
  }

  return clamp(width, MENU_MINIMUM_WIDTH, CANVAS_WIDTH - MENU_SIDE_MARGIN);
}

static void
draw_menu(
  const char *title,
  const char *const *items,
  int item_count,
  int selected_index,
  int visible_count
) {

  int panel_width = compute_menu_width(title, items, item_count);
  int panel_height = visible_count * WIDGET_ROW_HEIGHT + 18;
  int panel_x = (CANVAS_WIDTH - panel_width) / 2;
  int panel_y = (CANVAS_HEIGHT - panel_height) / 2;
  int top_index = selected_index - visible_count + 1;

  if (top_index < 0)
    top_index = 0;

  canvas_fill(THEME_BACKGROUND_COLOR);
  draw_titled_panel(panel_x, panel_y, panel_width, panel_height, title);

  for (int row = 0; row < visible_count; row++) {
    int index = top_index + row;
    int row_y = panel_y + 16 + row * WIDGET_ROW_HEIGHT;

    if (index == selected_index) {
      canvas_fill_rect(
        panel_x + 2,
        row_y,
        panel_width - 4,
        WIDGET_ROW_HEIGHT,
        THEME_BOX_COLOR
      );
    }

    canvas_text(
      panel_x + 8,
      compute_centered_text_y(row_y, WIDGET_ROW_HEIGHT),
      items[index],
      index == selected_index ? THEME_SELECTED_COLOR : THEME_TEXT_COLOR
    );
  }

  draw_scrollbar(
    panel_x + panel_width - 4,
    panel_y + 16,
    visible_count * WIDGET_ROW_HEIGHT,
    top_index,
    visible_count,
    item_count
  );

  canvas_push();
}

int
widget_run_menu(
  const char *title,
  const char *const *items,
  int item_count,
  int initial_index
) {

  if (item_count <= 0)
    return -1;

  int selected_index = clamp(initial_index, 0, item_count - 1);
  int visible_count =
    (CANVAS_HEIGHT - MENU_VERTICAL_MARGIN) / WIDGET_ROW_HEIGHT;

  if (visible_count > item_count)
    visible_count = item_count;

  for (;;) {
    draw_menu(title, items, item_count, selected_index, visible_count);

    int key = keyboard_wait_key();

    if (key == KEY_ESCAPE)
      return -1;

    if (key == KEY_ENTER)
      return selected_index;

    if (key == KEY_UP || key == 'k' || key == ';')
      selected_index = clamp(selected_index - 1, 0, item_count - 1);

    if (key == KEY_DOWN || key == 'j' || key == '.')
      selected_index = clamp(selected_index + 1, 0, item_count - 1);
  }
}

static void
draw_confirm(const char *question) {
  int panel_y = CANVAS_HEIGHT - CONFIRM_PANEL_BOTTOM_MARGIN;
  int two_line_height = WIDGET_ROW_HEIGHT + CANVAS_FONT_HEIGHT;
  int question_y = panel_y + (CONFIRM_PANEL_HEIGHT - two_line_height) / 2;

  canvas_fill(THEME_BACKGROUND_COLOR);
  widget_draw_panel(0, panel_y, CANVAS_WIDTH, CONFIRM_PANEL_HEIGHT);
  canvas_text(4, question_y, question, THEME_TEXT_COLOR);
  canvas_text(
    4,
    question_y + WIDGET_ROW_HEIGHT,
    CONFIRM_HINT,
    THEME_EMPHASIS_COLOR
  );
  canvas_push();
}

bool
widget_run_confirm(const char *question) {
  draw_confirm(question);

  for (;;) {
    int key = keyboard_wait_key();

    if (key == KEY_ESCAPE || key == 'n' || key == 'N')
      return false;

    if (key == 'y' || key == 'Y')
      return true;
  }
}
