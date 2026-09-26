#pragma once

#include <stdbool.h>
#include <stddef.h>

#define PINO_PIN_COUNT 40
#define PINO_GPIO_COUNT 30
#define PINO_LED_GPIO 25
#define PINO_VBUS_SENSE_GPIO 24
#define PINO_PROGRAM_FILENAME "app.rb"
#define PINO_PROGRAM_PATH SAVE_DIRECTORY_PATH "/" PINO_PROGRAM_FILENAME
#define PINO_CONSOLE_LINE_COUNT 8
#define PINO_CONSOLE_LINE_SIZE 36

typedef enum {
  PINO_DIRECTION_NONE,
  PINO_DIRECTION_IN,
  PINO_DIRECTION_OUT
} PinoDirection;

typedef enum {
  PINO_PULL_NONE,
  PINO_PULL_UP,
  PINO_PULL_DOWN
} PinoPull;

typedef enum {
  PINO_PROGRAM_STOPPED,
  PINO_PROGRAM_RUNNING,
  PINO_PROGRAM_FINISHED,
  PINO_PROGRAM_FAILED
} PinoProgramState;

const char *find_pino_pin_name(int terminal_index);
void reset_pino_gpios(void);
bool is_pino_led_lit(void);
bool is_pino_powered(void);
void reset_pino_power(void);

void init_pino_gpio(int gpio);
void set_pino_gpio_direction(int gpio, int direction);
void set_pino_gpio_pull(int gpio, int pull);
void write_pino_gpio(int gpio, int level);
int read_pino_gpio(int gpio);

void advance_pino_ticks(void);

void stop_pino_program(void);
void restart_pino_program(void);
void step_pino_program(void);
PinoProgramState read_pino_program_state(void);
void write_pino_console(const char *text, int byte_count);
int count_pino_console_lines(void);
const char *read_pino_console_line(int line_index);
