#pragma once

#define COUNT_PAGE_LINES(lines) ((int)(sizeof(lines) / sizeof((lines)[0])))

typedef enum {
  PAGE_LINE_BLANK,
  PAGE_LINE_HEADING,
  PAGE_LINE_TEXT,
  PAGE_LINE_KEY
} PageLineKind;

typedef struct {
  PageLineKind kind;
  const char *key;
  const char *text;
} PageLine;

void show_page(const char *title, const PageLine *lines, int line_count);
