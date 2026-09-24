#pragma once

#include <stdbool.h>

#define WIDGET_HEADER_HEIGHT 13
#define WIDGET_ROW_HEIGHT 16

void widget_draw_header(const char *title, const char *right_text);
void widget_draw_panel(int x, int y, int width, int height);
int widget_run_menu(
  const char *title,
  const char *const *items,
  int item_count,
  int initial_index
);
bool widget_run_confirm(const char *question);
