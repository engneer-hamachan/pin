#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CANVAS_WIDTH 240
#define CANVAS_HEIGHT 135
#define CANVAS_FONT_HEIGHT 13

void canvas_begin(void);
void canvas_fill(uint32_t color);
void canvas_pixel(int x, int y, uint32_t color);
void canvas_line(int x0, int y0, int x1, int y1, uint32_t color);
void canvas_rect(int x, int y, int width, int height, uint32_t color);
void canvas_fill_rect(int x, int y, int width, int height, uint32_t color);
void canvas_circle(int x, int y, int radius, uint32_t color);
void canvas_fill_circle(int x, int y, int radius, uint32_t color);
void canvas_text(int x, int y, const char *text, uint32_t color);
int canvas_text_width(const char *text);
void canvas_push(void);

#ifdef __cplusplus
}
#endif
