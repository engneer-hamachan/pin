#include "keyboard.h"

#include <M5Unified.hpp>
#include <stdint.h>
#include <string.h>

#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static constexpr int KEY_CODE_COUNT = 128;
static constexpr int KEY_QUEUE_CAPACITY = 16;
static constexpr int REPEAT_DELAY_MS = 450;
static constexpr int REPEAT_INTERVAL_MS = 55;
static constexpr int REPEAT_TIMEOUT_MS = 2000;
static constexpr int WAIT_POLL_INTERVAL_MS = 10;

enum class KeyKind : uint8_t {
  None,
  Character,
  Shift,
  Fn,
  Ctrl,
  Modifier,
  Enter,
  Tab,
  Backspace,
  Space
};

struct KeyboardKey {
  KeyKind kind;
  char character;
};

static bool shift_pressed = false;
static bool fn_pressed = false;
static bool ctrl_pressed = false;
static bool key_down[KEY_CODE_COUNT] = {};
static uint8_t repeat_code = 0;
static TickType_t repeat_next_tick = 0;
static TickType_t repeat_start_tick = 0;
static int key_queue[KEY_QUEUE_CAPACITY];
static int key_queue_head = 0;
static int key_queue_count = 0;

static KeyboardKey
lookup_key(uint8_t code) {
  switch (code) {
  case 1:
    return {KeyKind::Character, '`'};
  case 2:
    return {KeyKind::Tab, 0};
  case 3:
    return {KeyKind::Fn, 0};
  case 4:
    return {KeyKind::Ctrl, 0};
  case 5:
    return {KeyKind::Character, '1'};
  case 6:
    return {KeyKind::Character, 'q'};
  case 7:
    return {KeyKind::Shift, 0};
  case 8:
    return {KeyKind::Modifier, 0};
  case 11:
    return {KeyKind::Character, '2'};
  case 12:
    return {KeyKind::Character, 'w'};
  case 13:
    return {KeyKind::Character, 'a'};
  case 14:
    return {KeyKind::Modifier, 0};
  case 15:
    return {KeyKind::Character, '3'};
  case 16:
    return {KeyKind::Character, 'e'};
  case 17:
    return {KeyKind::Character, 's'};
  case 18:
    return {KeyKind::Character, 'z'};
  case 21:
    return {KeyKind::Character, '4'};
  case 22:
    return {KeyKind::Character, 'r'};
  case 23:
    return {KeyKind::Character, 'd'};
  case 24:
    return {KeyKind::Character, 'x'};
  case 25:
    return {KeyKind::Character, '5'};
  case 26:
    return {KeyKind::Character, 't'};
  case 27:
    return {KeyKind::Character, 'f'};
  case 28:
    return {KeyKind::Character, 'c'};
  case 31:
    return {KeyKind::Character, '6'};
  case 32:
    return {KeyKind::Character, 'y'};
  case 33:
    return {KeyKind::Character, 'g'};
  case 34:
    return {KeyKind::Character, 'v'};
  case 35:
    return {KeyKind::Character, '7'};
  case 36:
    return {KeyKind::Character, 'u'};
  case 37:
    return {KeyKind::Character, 'h'};
  case 38:
    return {KeyKind::Character, 'b'};
  case 41:
    return {KeyKind::Character, '8'};
  case 42:
    return {KeyKind::Character, 'i'};
  case 43:
    return {KeyKind::Character, 'j'};
  case 44:
    return {KeyKind::Character, 'n'};
  case 45:
    return {KeyKind::Character, '9'};
  case 46:
    return {KeyKind::Character, 'o'};
  case 47:
    return {KeyKind::Character, 'k'};
  case 48:
    return {KeyKind::Character, 'm'};
  case 51:
    return {KeyKind::Character, '0'};
  case 52:
    return {KeyKind::Character, 'p'};
  case 53:
    return {KeyKind::Character, 'l'};
  case 54:
    return {KeyKind::Character, ','};
  case 55:
    return {KeyKind::Character, '_'};
  case 56:
    return {KeyKind::Character, '['};
  case 57:
    return {KeyKind::Character, ';'};
  case 58:
    return {KeyKind::Character, '.'};
  case 61:
    return {KeyKind::Character, '='};
  case 62:
    return {KeyKind::Character, ']'};
  case 63:
    return {KeyKind::Character, '\''};
  case 64:
    return {KeyKind::Character, '/'};
  case 65:
    return {KeyKind::Backspace, 0};
  case 66:
    return {KeyKind::Character, '\\'};
  case 67:
    return {KeyKind::Enter, 0};
  case 68:
    return {KeyKind::Space, 0};
  default:
    return {KeyKind::None, 0};
  }
}

static char
apply_shift(char character) {
  if (character >= 'a' && character <= 'z') {
    return (char)(character - 'a' + 'A');
  }

  switch (character) {
  case '`':
    return '~';
  case '1':
    return '!';
  case '2':
    return '@';
  case '3':
    return '#';
  case '4':
    return '$';
  case '5':
    return '%';
  case '6':
    return '^';
  case '7':
    return '&';
  case '8':
    return '*';
  case '9':
    return '(';
  case '0':
    return ')';
  case '_':
    return '-';
  case '=':
    return '+';
  case '[':
    return '{';
  case ']':
    return '}';
  case ';':
    return ':';
  case '\'':
    return '"';
  case ',':
    return '<';
  case '.':
    return '>';
  case '/':
    return '?';
  case '\\':
    return '|';
  default:
    return character;
  }
}

static int
find_fn_arrow_key(char character) {
  switch (character) {
  case ';':
    return KEY_UP;
  case '.':
    return KEY_DOWN;
  case ',':
    return KEY_LEFT;
  case '/':
    return KEY_RIGHT;
  default:
    return KEY_NONE;
  }
}

static void
push_key(int key) {
  if (key_queue_count == KEY_QUEUE_CAPACITY) {
    return;
  }

  key_queue[(key_queue_head + key_queue_count) % KEY_QUEUE_CAPACITY] = key;
  key_queue_count++;
}

static int
pop_key(void) {
  if (key_queue_count == 0) {
    return KEY_NONE;
  }

  int key = key_queue[key_queue_head];

  key_queue_head = (key_queue_head + 1) % KEY_QUEUE_CAPACITY;
  key_queue_count--;

  return key;
}

static bool
is_repeatable_key(KeyKind kind) {
  switch (kind) {
  case KeyKind::Character:
  case KeyKind::Enter:
  case KeyKind::Tab:
  case KeyKind::Backspace:
  case KeyKind::Space:
    return true;
  default:
    return false;
  }
}

static void
clear_repeat_tracking(void) {
  repeat_code = 0;
  repeat_next_tick = 0;
  repeat_start_tick = 0;
}

static void
start_repeat(uint8_t code) {
  if (!is_repeatable_key(lookup_key(code).kind)) {
    return;
  }

  repeat_code = code;
  repeat_start_tick = xTaskGetTickCount();
  repeat_next_tick = repeat_start_tick + pdMS_TO_TICKS(REPEAT_DELAY_MS);
}

static void
stop_repeat(uint8_t code) {
  if (repeat_code != code) {
    return;
  }

  clear_repeat_tracking();

  for (int i = 1; i < KEY_CODE_COUNT; i++) {
    if (key_down[i] && is_repeatable_key(lookup_key((uint8_t)i).kind)) {
      start_repeat((uint8_t)i);
      return;
    }
  }
}

static void
push_pressed_key(uint8_t code) {
  KeyboardKey key = lookup_key(code);

  switch (key.kind) {
  case KeyKind::Enter:
    push_key(KEY_ENTER);
    return;
  case KeyKind::Tab:
    push_key('\t');
    return;
  case KeyKind::Backspace:
    push_key(KEY_BACKSPACE);
    return;
  case KeyKind::Space:
    push_key(' ');
    return;
  case KeyKind::Character:
    break;
  default:
    return;
  }

  if (fn_pressed) {
    int arrow_key = find_fn_arrow_key(key.character);

    if (arrow_key != KEY_NONE) {
      push_key(arrow_key);
      return;
    }
  }

  if (ctrl_pressed && key.character >= 'a' && key.character <= 'z') {
    push_key(key.character & 0x1F);
    return;
  }

  char character = shift_pressed ? apply_shift(key.character) : key.character;

  if (character == '`') {
    push_key(KEY_ESCAPE);
    return;
  }

  push_key(character);
}

static void
handle_key_event(uint8_t code, bool pressed) {
  switch (lookup_key(code).kind) {
  case KeyKind::Shift:
    shift_pressed = pressed;
    return;
  case KeyKind::Fn:
    fn_pressed = pressed;
    return;
  case KeyKind::Ctrl:
    ctrl_pressed = pressed;
    return;
  case KeyKind::Modifier:
  case KeyKind::None:
    return;
  default:
    break;
  }

  if (pressed) {
    push_pressed_key(code);
  }
}

static void
change_key_state(uint8_t code, bool pressed) {
  key_down[code] = pressed;

  if (pressed) {
    start_repeat(code);
  } else {
    stop_repeat(code);
  }

  handle_key_event(code, pressed);
}

static void
repeat_held_key(void) {
  if (repeat_code == 0 || !key_down[repeat_code]) {
    return;
  }

  TickType_t now = xTaskGetTickCount();

  if ((int32_t)(now - repeat_start_tick) >=
      (int32_t)pdMS_TO_TICKS(REPEAT_TIMEOUT_MS)) {

    key_down[repeat_code] = false;
    clear_repeat_tracking();
    return;
  }

  if ((int32_t)(now - repeat_next_tick) < 0) {
    return;
  }

  handle_key_event(repeat_code, true);
  repeat_next_tick = now + pdMS_TO_TICKS(REPEAT_INTERVAL_MS);
}

#if defined(PIN_BOARD_CARDPUTER)

static const gpio_num_t ROW_SELECT_PINS[3] = {
  GPIO_NUM_8,
  GPIO_NUM_9,
  GPIO_NUM_11,
};

static const gpio_num_t COLUMN_PINS[7] = {
  GPIO_NUM_13,
  GPIO_NUM_15,
  GPIO_NUM_3,
  GPIO_NUM_4,
  GPIO_NUM_5,
  GPIO_NUM_6,
  GPIO_NUM_7,
};

static void
begin_scanner(void) {
  gpio_config_t output_config = {};

  output_config.mode = GPIO_MODE_OUTPUT;

  for (int i = 0; i < 3; i++) {
    output_config.pin_bit_mask |= 1ULL << ROW_SELECT_PINS[i];
  }

  gpio_config(&output_config);

  for (int i = 0; i < 3; i++) {
    gpio_set_level(ROW_SELECT_PINS[i], 0);
  }

  gpio_config_t input_config = {};

  input_config.mode = GPIO_MODE_INPUT;
  input_config.pull_up_en = GPIO_PULLUP_ENABLE;

  for (int i = 0; i < 7; i++) {
    input_config.pin_bit_mask |= 1ULL << COLUMN_PINS[i];
  }

  gpio_config(&input_config);
}

static uint8_t
compute_key_code(int select_index, int column_pin_index) {
  int row = 3 - (select_index > 3 ? select_index - 4 : select_index);
  int column = column_pin_index * 2 + (select_index > 3 ? 0 : 1);

  return (uint8_t)((column / 2) * 10 + 1 + (column % 2) * 4 + row);
}

static void
scan_keyboard(void) {
  bool scanned_down[KEY_CODE_COUNT] = {};

  for (int i = 0; i < 8; i++) {
    gpio_set_level(ROW_SELECT_PINS[0], (i >> 0) & 1);
    gpio_set_level(ROW_SELECT_PINS[1], (i >> 1) & 1);
    gpio_set_level(ROW_SELECT_PINS[2], (i >> 2) & 1);
    esp_rom_delay_us(5);

    for (int j = 0; j < 7; j++) {
      if (gpio_get_level(COLUMN_PINS[j]) == 0) {
        scanned_down[compute_key_code(i, j)] = true;
      }
    }
  }

  for (int i = 0; i < 3; i++) {
    gpio_set_level(ROW_SELECT_PINS[i], 0);
  }

  for (int code = 0; code < KEY_CODE_COUNT; code++) {
    if (scanned_down[code] != key_down[code]) {
      change_key_state((uint8_t)code, scanned_down[code]);
    }
  }

  repeat_held_key();
}

#else

static constexpr uint8_t TCA8418_ADDRESS = 0x34;
static constexpr uint32_t TCA8418_FREQUENCY = 400000;
static constexpr uint8_t TCA8418_REGISTER_CONFIGURATION = 0x01;
static constexpr uint8_t TCA8418_REGISTER_INTERRUPT_STATUS = 0x02;
static constexpr uint8_t TCA8418_REGISTER_KEY_EVENT_A = 0x04;
static constexpr uint8_t TCA8418_REGISTER_KEYPAD_GPIO1 = 0x1D;
static constexpr uint8_t TCA8418_REGISTER_KEYPAD_GPIO2 = 0x1E;
static constexpr uint8_t TCA8418_REGISTER_KEYPAD_GPIO3 = 0x1F;
static constexpr uint8_t TCA8418_CONFIGURATION_AUTO_INCREMENT = 0x80;
static constexpr uint8_t TCA8418_CONFIGURATION_KEY_EVENT_INTERRUPT = 0x01;
static constexpr uint8_t TCA8418_INTERRUPT_KEY = 0x01;
static constexpr uint8_t TCA8418_INTERRUPT_OVERFLOW = 0x08;
static constexpr uint8_t TCA8418_KEY_CODE_MASK = 0x7F;
static constexpr uint8_t TCA8418_KEY_PRESSED_BIT = 0x80;
static constexpr int TCA8418_FIFO_DEPTH = 10;
static constexpr int TCA8418_RECONFIGURE_AFTER_ERROR_COUNT = 8;

static int i2c_error_count = 0;

static bool
write_tca8418_register(uint8_t register_address, uint8_t value) {
  if (!M5.In_I2C.start(TCA8418_ADDRESS, false, TCA8418_FREQUENCY)) {
    return false;
  }

  uint8_t bytes[2] = {register_address, value};
  bool write_succeeded = M5.In_I2C.write(bytes, 2);

  M5.In_I2C.stop();

  return write_succeeded;
}

static bool
read_tca8418_register(uint8_t register_address, uint8_t *value) {
  if (!M5.In_I2C.start(TCA8418_ADDRESS, false, TCA8418_FREQUENCY)) {
    return false;
  }

  bool write_succeeded = M5.In_I2C.write(&register_address, 1);

  M5.In_I2C.stop();

  if (!write_succeeded) {
    return false;
  }

  if (!M5.In_I2C.start(TCA8418_ADDRESS, true, TCA8418_FREQUENCY)) {
    return false;
  }

  bool read_succeeded = M5.In_I2C.read(value, 1, true);

  M5.In_I2C.stop();

  return read_succeeded;
}

static void
release_all_keys(void) {
  shift_pressed = false;
  fn_pressed = false;
  memset(key_down, 0, sizeof(key_down));
  clear_repeat_tracking();
}

static void
begin_scanner(void) {
  write_tca8418_register(TCA8418_REGISTER_KEYPAD_GPIO1, 0xFF);
  write_tca8418_register(TCA8418_REGISTER_KEYPAD_GPIO2, 0xFF);
  write_tca8418_register(TCA8418_REGISTER_KEYPAD_GPIO3, 0x03);
  write_tca8418_register(
    TCA8418_REGISTER_CONFIGURATION,
    (uint8_t)(TCA8418_CONFIGURATION_AUTO_INCREMENT |
              TCA8418_CONFIGURATION_KEY_EVENT_INTERRUPT)
  );
  write_tca8418_register(TCA8418_REGISTER_INTERRUPT_STATUS, 0xFF);

  for (int i = 0; i < TCA8418_FIFO_DEPTH; i++) {
    uint8_t event = 0;

    if (!read_tca8418_register(TCA8418_REGISTER_KEY_EVENT_A, &event) ||
        event == 0) {
      break;
    }
  }

  write_tca8418_register(TCA8418_REGISTER_INTERRUPT_STATUS, 0xFF);
  release_all_keys();
  i2c_error_count = 0;
}

static void
count_i2c_error(void) {
  memset(key_down, 0, sizeof(key_down));
  clear_repeat_tracking();

  if (++i2c_error_count >= TCA8418_RECONFIGURE_AFTER_ERROR_COUNT) {
    begin_scanner();
  }
}

static void
scan_keyboard(void) {
  uint8_t interrupt_status = 0;

  if (!read_tca8418_register(
        TCA8418_REGISTER_INTERRUPT_STATUS,
        &interrupt_status
      )) {

    count_i2c_error();
    return;
  }

  for (int i = 0; i < TCA8418_FIFO_DEPTH; i++) {
    uint8_t event = 0;

    if (!read_tca8418_register(TCA8418_REGISTER_KEY_EVENT_A, &event)) {
      count_i2c_error();
      return;
    }

    if (event == 0) {
      break;
    }

    change_key_state(
      (uint8_t)(event & TCA8418_KEY_CODE_MASK),
      (event & TCA8418_KEY_PRESSED_BIT) != 0
    );
  }

  if (interrupt_status & TCA8418_INTERRUPT_OVERFLOW) {
    release_all_keys();
  }

  if (interrupt_status & (TCA8418_INTERRUPT_KEY | TCA8418_INTERRUPT_OVERFLOW)) {
    write_tca8418_register(
      TCA8418_REGISTER_INTERRUPT_STATUS,
      (uint8_t)(TCA8418_INTERRUPT_KEY | TCA8418_INTERRUPT_OVERFLOW)
    );
  }

  i2c_error_count = 0;
  repeat_held_key();
}

#endif

void
keyboard_begin(void) {
  begin_scanner();
}

int
keyboard_read_key(void) {
  scan_keyboard();

  return pop_key();
}

int
keyboard_wait_key(void) {
  for (;;) {
    int key = keyboard_read_key();

    if (key != KEY_NONE) {
      return key;
    }

    vTaskDelay(pdMS_TO_TICKS(WAIT_POLL_INTERVAL_MS));
  }
}

void
keyboard_set_text_input(bool enabled) {
  (void)enabled;
}
