#include "page.h"

#include "breadboard.h"
#include "canvas.h"
#include "keyboard.h"
#include "theme.h"

#define PAGE_TEXT_X 8
#define PAGE_KEY_TEXT_X 56
#define PAGE_TITLE_Y 2
#define PAGE_TITLE_RULE_Y 16
#define PAGE_BODY_Y 20
#define PAGE_LINE_HEIGHT 12
#define PAGE_VISIBLE_LINE_COUNT 8
#define PAGE_BODY_HEIGHT (PAGE_VISIBLE_LINE_COUNT * PAGE_LINE_HEIGHT)
#define PAGE_HEADING_GAP 2
#define PAGE_FOOTER_RULE_Y 119
#define PAGE_FOOTER_Y 121
#define PAGE_SCROLLBAR_X 234
#define PAGE_SCROLLBAR_WIDTH 2
#define PAGE_TEXT_COLOR 0xF4E8D0
#define PAGE_HEADING_COLOR 0xF8C020
#define PAGE_KEY_COLOR 0xF0501A
#define PAGE_RULE_COLOR 0xF0501A
#define PAGE_HINT_COLOR 0x8A8A8A
#define PAGE_SCROLLBAR_TRACK_COLOR 0x303030
#define PAGE_SCROLLBAR_THUMB_COLOR 0xF4E8D0

static void
draw_page_line(const PageLine *line, int y) {
  switch (line->kind) {
  case PAGE_LINE_HEADING:
    canvas_text(PAGE_TEXT_X, y, line->text, PAGE_HEADING_COLOR);
    break;
  case PAGE_LINE_TEXT:
    canvas_text(PAGE_TEXT_X, y, line->text, PAGE_TEXT_COLOR);
    break;
  case PAGE_LINE_KEY:
    canvas_text(PAGE_TEXT_X, y, line->key, PAGE_KEY_COLOR);
    canvas_text(PAGE_KEY_TEXT_X, y, line->text, PAGE_TEXT_COLOR);
    break;
  default:
    break;
  }
}

static int
compute_page_line_height(const PageLine *line) {
  if (line->kind == PAGE_LINE_HEADING)
    return PAGE_LINE_HEIGHT + PAGE_HEADING_GAP;

  return PAGE_LINE_HEIGHT;
}

static int
find_last_top_index(const PageLine *lines, int line_count) {
  int top_index = line_count;
  int height = 0;

  while (top_index > 0 &&
         height + compute_page_line_height(&lines[top_index - 1]) <=
           PAGE_BODY_HEIGHT) {
    height += compute_page_line_height(&lines[top_index - 1]);
    top_index--;
  }

  return top_index;
}

static void
draw_page_scrollbar(const PageLine *lines, int line_count, int top_index) {
  int last_top_index = find_last_top_index(lines, line_count);

  if (last_top_index == 0)
    return;

  int visible_line_count = line_count - last_top_index;
  int thumb_height = PAGE_BODY_HEIGHT * visible_line_count / line_count;
  int thumb_y = PAGE_BODY_Y +
                (PAGE_BODY_HEIGHT - thumb_height) * top_index / last_top_index;

  canvas_fill_rect(
    PAGE_SCROLLBAR_X,
    PAGE_BODY_Y,
    PAGE_SCROLLBAR_WIDTH,
    PAGE_BODY_HEIGHT,
    PAGE_SCROLLBAR_TRACK_COLOR
  );
  canvas_fill_rect(
    PAGE_SCROLLBAR_X,
    thumb_y,
    PAGE_SCROLLBAR_WIDTH,
    thumb_height,
    PAGE_SCROLLBAR_THUMB_COLOR
  );
}

static void
draw_page(
  const char *title,
  const PageLine *lines,
  int line_count,
  int top_index
) {

  canvas_fill(THEME_BACKGROUND_COLOR);
  canvas_text(PAGE_TEXT_X, PAGE_TITLE_Y, title, PAGE_TEXT_COLOR);
  canvas_fill_rect(0, PAGE_TITLE_RULE_Y, CANVAS_WIDTH, 2, PAGE_RULE_COLOR);

  int y = PAGE_BODY_Y;

  for (int i = top_index; i < line_count; i++) {
    int line_height = compute_page_line_height(&lines[i]);

    if (y + line_height > PAGE_BODY_Y + PAGE_BODY_HEIGHT)
      break;

    draw_page_line(&lines[i], y);
    y += line_height;
  }

  draw_page_scrollbar(lines, line_count, top_index);
  canvas_line(
    0,
    PAGE_FOOTER_RULE_Y,
    CANVAS_WIDTH - 1,
    PAGE_FOOTER_RULE_Y,
    PAGE_RULE_COLOR
  );
  canvas_text(
    PAGE_TEXT_X,
    PAGE_FOOTER_Y,
    "j/k line  h/l page  other key back",
    PAGE_HINT_COLOR
  );
  canvas_push();
}

void
show_page(const char *title, const PageLine *lines, int line_count) {
  int top_index = 0;

  for (;;) {
    draw_page(title, lines, line_count, top_index);

    switch (decode_key(keyboard_wait_key())) {
    case KEY_UP:
      top_index--;
      break;
    case KEY_DOWN:
      top_index++;
      break;
    case KEY_LEFT:
      top_index -= PAGE_VISIBLE_LINE_COUNT;
      break;
    case KEY_RIGHT:
      top_index += PAGE_VISIBLE_LINE_COUNT;
      break;
    default:
      return;
    }

    top_index =
      clamp_integer(top_index, 0, find_last_top_index(lines, line_count));
  }
}
