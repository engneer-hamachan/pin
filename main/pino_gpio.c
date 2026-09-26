#include "pino.h"

#include "gpio.h"

#include <stdlib.h>
#include <string.h>

int
GPIO_pin_num_from_char(const uint8_t *str) {
  const char *text = (const char *)str;
  char *end;

  if (strncmp(text, "GPIO", 4) == 0)
    text += 4;
  else if (strncmp(text, "GP", 2) == 0)
    text += 2;

  long gpio = strtol(text, &end, 10);

  if (end == text || *end != '\0' || gpio < 0 || gpio >= PINO_GPIO_COUNT)
    return -1;

  return (int)gpio;
}

void
GPIO_init(uint8_t pin) {
  advance_pino_ticks();
  init_pino_gpio(pin);
}

void
GPIO_set_dir(uint8_t pin, uint8_t dir) {
  advance_pino_ticks();

  switch (dir) {
  case IN:
    set_pino_gpio_direction(pin, PINO_DIRECTION_IN);
    break;
  case OUT:
    set_pino_gpio_direction(pin, PINO_DIRECTION_OUT);
    break;
  default:
    set_pino_gpio_direction(pin, PINO_DIRECTION_NONE);
    break;
  }
}

void
GPIO_pull_up(uint8_t pin) {
  advance_pino_ticks();
  set_pino_gpio_pull(pin, PINO_PULL_UP);
}

void
GPIO_pull_down(uint8_t pin) {
  advance_pino_ticks();
  set_pino_gpio_pull(pin, PINO_PULL_DOWN);
}

void
GPIO_open_drain(uint8_t pin) {
  (void)pin;
  advance_pino_ticks();
}

int
GPIO_read(uint8_t pin) {
  advance_pino_ticks();
  return read_pino_gpio(pin);
}

void
GPIO_write(uint8_t pin, uint8_t val) {
  advance_pino_ticks();
  write_pino_gpio(pin, val);
}

void
GPIO_set_function(uint8_t pin, uint8_t function) {
  (void)pin;
  (void)function;
  advance_pino_ticks();
}
