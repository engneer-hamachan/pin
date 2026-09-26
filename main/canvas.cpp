#include "canvas.h"

#include "theme.h"

#include <M5Unified.hpp>
#include <stdlib.h>

#if defined(PIN_BOARD_TFT_ST7789) || defined(PIN_BOARD_TFT_ILI9341)
#include "driver/gpio.h"
#include <lgfx/v1/platforms/esp32/Bus_SPI.hpp>
#if defined(PIN_BOARD_TFT_ILI9341)
#include "Panel_ILI9341.hpp"
#else
#include <lgfx/v1/panel/Panel_ST7789.hpp>
#endif
#endif

static lgfx::LGFX_Device *display = nullptr;
static lgfx::LGFX_Sprite *sprite = nullptr;
static lgfx::LGFX_Sprite *row_sprite = nullptr;

#if defined(PIN_BOARD_TFT_ST7789) || defined(PIN_BOARD_TFT_ILI9341)

static constexpr int PANEL_WIDTH = 240;
static constexpr int PANEL_HEIGHT = 320;
static constexpr int PANEL_ROTATION = 1;
static constexpr gpio_num_t SD_CARD_CHIP_SELECT_PIN = GPIO_NUM_12;

static lgfx::Bus_SPI panel_bus;
#if defined(PIN_BOARD_TFT_ILI9341)
static lgfx::Panel_ILI9341 panel;
#else
static lgfx::Panel_ST7789 panel;
#endif
static lgfx::LGFX_Device external_display;

static void
configure_external_display(void) {
  auto bus_config = panel_bus.config();

  bus_config.spi_host = SPI2_HOST;
  bus_config.pin_sclk = 40;
  bus_config.pin_mosi = 14;
  bus_config.pin_miso = 39;
  bus_config.use_lock = true;
  bus_config.spi_mode = 0;
  bus_config.freq_write = 40000000;
  bus_config.spi_3wire = false;
  bus_config.pin_dc = 6;
  panel_bus.config(bus_config);
  panel.setBus(&panel_bus);

  auto panel_config = panel.config();

  panel_config.pin_cs = 5;
  panel_config.pin_rst = 3;
  panel_config.memory_width = PANEL_WIDTH;
  panel_config.memory_height = PANEL_HEIGHT;
  panel_config.panel_width = PANEL_WIDTH;
  panel_config.panel_height = PANEL_HEIGHT;
  panel_config.offset_x = 0;
  panel_config.offset_y = 0;
#if defined(PIN_BOARD_TFT_ILI9341)
  panel_config.offset_rotation = 2;
#endif
  panel_config.invert = false;
  panel_config.rgb_order = false;
  panel_config.readable = false;
  panel_config.bus_shared = true;
  panel.config(panel_config);
  external_display.setPanel(&panel);
}

static lgfx::LGFX_Device *
begin_display(void) {
  M5.Display.setBrightness(0);
  M5.Display.sleep();
  M5.Display.waitDisplay();
  M5.Display.releaseBus();

  gpio_set_level(SD_CARD_CHIP_SELECT_PIN, 1);
  gpio_set_direction(SD_CARD_CHIP_SELECT_PIN, GPIO_MODE_OUTPUT);

  configure_external_display();
  external_display.init();
  external_display.setColorDepth(16);
  external_display.setRotation(PANEL_ROTATION);

  return &external_display;
}

#else

static constexpr int INTERNAL_DISPLAY_BRIGHTNESS = 90;

static lgfx::LGFX_Device *
begin_display(void) {
  M5.Display.setBrightness(INTERNAL_DISPLAY_BRIGHTNESS);

  return &M5.Display;
}

#endif

void
canvas_begin(void) {
  auto config = M5.config();
  M5.begin(config);

  display = begin_display();
  display->fillScreen((uint32_t)THEME_BACKGROUND_COLOR);

  sprite = new lgfx::LGFX_Sprite(display);
  sprite->setPsram(false);
  sprite->setColorDepth(16);

  if (sprite->createSprite(CANVAS_WIDTH, CANVAS_HEIGHT) == nullptr) {
    abort();
  }

  sprite->setFont(&lgfx::v1::fonts::efontJA_12);
  sprite->setTextSize(1);
  sprite->setTextDatum(lgfx::v1::textdatum_t::top_left);

  row_sprite = new lgfx::LGFX_Sprite(sprite);
  row_sprite->setPsram(false);
  row_sprite->setColorDepth(16);

  if (row_sprite->createSprite(CANVAS_WIDTH, CANVAS_ROW_HEIGHT) == nullptr) {
    abort();
  }

  row_sprite->setFont(&lgfx::v1::fonts::efontJA_12);
  row_sprite->setTextSize(1);
  row_sprite->setTextDatum(lgfx::v1::textdatum_t::top_left);
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

void
canvas_push(void) {
  sprite->pushSprite(
    display,
    (display->width() - CANVAS_WIDTH) / 2,
    (display->height() - CANVAS_HEIGHT) / 2
  );
}

void
canvas_row_fill(uint32_t color) {
  row_sprite->fillScreen(color);
}

void
canvas_row_fill_rect(int x, int y, int width, int height, uint32_t color) {
  row_sprite->fillRect(x, y, width, height, color);
}

void
canvas_row_line(int x0, int y0, int x1, int y1, uint32_t color) {
  row_sprite->drawLine(x0, y0, x1, y1, color);
}

void
canvas_row_text(int x, int y, const char *text, uint32_t color) {
  row_sprite->setTextColor(color);
  row_sprite->drawString(text, x, y);
}

void
canvas_row_commit(int y) {
  row_sprite->pushSprite(sprite, 0, y);
}
