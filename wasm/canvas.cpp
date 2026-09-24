#include "canvas.h"

#include "theme.h"

#include <emscripten.h>
#include <lgfx/v1/LGFX_Sprite.hpp>
#include <stdlib.h>

static constexpr int RGBA_BYTE_COUNT = 4;
static constexpr int PIXEL_COUNT = CANVAS_WIDTH * CANVAS_HEIGHT;

static lgfx::LGFX_Sprite *sprite = nullptr;
static uint8_t rgba_pixels[PIXEL_COUNT * RGBA_BYTE_COUNT];

EM_JS(void, put_canvas_pixels, (const uint8_t *pixels, int width, int height), {
  const canvas = document.getElementById("canvas");
  const bytes = new Uint8ClampedArray(HEAPU8.buffer, pixels, width * height * 4);

  canvas.getContext("2d").putImageData(new ImageData(bytes, width, height), 0, 0);
});

void
canvas_begin(void) {
  sprite = new lgfx::LGFX_Sprite();
  sprite->setColorDepth(16);

  if (sprite->createSprite(CANVAS_WIDTH, CANVAS_HEIGHT) == nullptr) {
    abort();
  }

  sprite->setFont(&lgfx::v1::fonts::efontJA_12);
  sprite->setTextSize(1);
  sprite->setTextDatum(lgfx::v1::textdatum_t::top_left);
  sprite->fillScreen((uint32_t)THEME_BACKGROUND_COLOR);
}

void
canvas_fill(uint32_t color) {
  sprite->fillScreen(color);
}

void
canvas_pixel(int x, int y, uint32_t color) {
  sprite->drawPixel(x, y, color);
}

void
canvas_line(int x0, int y0, int x1, int y1, uint32_t color) {
  sprite->drawLine(x0, y0, x1, y1, color);
}

void
canvas_rect(int x, int y, int width, int height, uint32_t color) {
  sprite->drawRect(x, y, width, height, color);
}

void
canvas_fill_rect(int x, int y, int width, int height, uint32_t color) {
  sprite->fillRect(x, y, width, height, color);
}

void
canvas_circle(int x, int y, int radius, uint32_t color) {
  sprite->drawCircle(x, y, radius, color);
}

void
canvas_fill_circle(int x, int y, int radius, uint32_t color) {
  sprite->fillCircle(x, y, radius, color);
}

void
canvas_text(int x, int y, const char *text, uint32_t color) {
  sprite->setTextColor(color);
  sprite->drawString(text, x, y);
}

int
canvas_text_width(const char *text) {
  return (int)sprite->textWidth(text);
}

// The 16-bit sprite stores each pixel as RRRRRGGG GGGBBBBB.
void
canvas_push(void) {
  const uint8_t *source = (const uint8_t *)sprite->getBuffer();

  for (int i = 0; i < PIXEL_COUNT; i++) {
    uint8_t high = source[i * 2];
    uint8_t low = source[i * 2 + 1];
    uint8_t red = high >> 3;
    uint8_t green = ((high & 0x07) << 3) | (low >> 5);
    uint8_t blue = low & 0x1F;
    uint8_t *target = &rgba_pixels[i * RGBA_BYTE_COUNT];

    target[0] = (red << 3) | (red >> 2);
    target[1] = (green << 2) | (green >> 4);
    target[2] = (blue << 3) | (blue >> 2);
    target[3] = 0xFF;
  }

  put_canvas_pixels(rgba_pixels, CANVAS_WIDTH, CANVAS_HEIGHT);
}
