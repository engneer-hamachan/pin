#include "pino.h"

#include "hal.h"
#include "machine.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#if defined(PIN_BOARD_WASM)
#include <emscripten.h>
#else
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "rom/ets_sys.h"
#endif

#define MICROSECONDS_PER_MILLISECOND 1000
#define TICK_MICROSECONDS (MRB_TICK_UNIT * MICROSECONDS_PER_MILLISECOND)
#define INSTRUCTIONS_PER_TICK_CHECK 256

volatile int sigint_status = MACHINE_SIG_NONE;

static mrb_state *volatile ticking_mrb = NULL;

#if defined(PIN_BOARD_WASM)

static uint64_t last_tick_us = 0;

uint64_t
Machine_uptime_us(void) {
  return (uint64_t)(emscripten_get_now() * MICROSECONDS_PER_MILLISECOND);
}

static void
wait_microseconds(uint64_t microseconds) {
  uint64_t start_us = Machine_uptime_us();

  while (Machine_uptime_us() - start_us < microseconds) {
  }
}

void
advance_pino_ticks(void) {
  if (ticking_mrb == NULL)
    return;

  uint64_t now_us = Machine_uptime_us();

  while (now_us - last_tick_us >= TICK_MICROSECONDS) {
    mrb_tick(ticking_mrb);
    last_tick_us += TICK_MICROSECONDS;
  }
}

static void
advance_ticks_on_instruction(
  mrb_state *mrb,
  const struct mrb_irep *irep,
  const mrb_code *pc,
  mrb_value *regs
) {
  static int instruction_count = 0;

  (void)mrb;
  (void)irep;
  (void)pc;
  (void)regs;

  instruction_count++;

  if (instruction_count < INSTRUCTIONS_PER_TICK_CHECK)
    return;

  instruction_count = 0;
  advance_pino_ticks();
}

void
picorb_hal_init(mrb_state *mrb) {
  ticking_mrb = mrb;
  last_tick_us = Machine_uptime_us();
  mrb->code_fetch_hook = advance_ticks_on_instruction;
}

void
picorb_hal_idle_cpu(mrb_state *mrb) {
  (void)mrb;
}

void
picorb_hal_sleep_us(mrb_state *mrb, mrb_int usec) {
  (void)mrb;
  wait_microseconds((uint64_t)usec);
}

void
Machine_delay_ms(uint32_t ms) {
  wait_microseconds((uint64_t)ms * MICROSECONDS_PER_MILLISECOND);
}

void
Machine_busy_wait_ms(uint32_t ms) {
  wait_microseconds((uint64_t)ms * MICROSECONDS_PER_MILLISECOND);
}

void
Machine_busy_wait_us(uint32_t us) {
  wait_microseconds(us);
}

void
picorb_hal_enable_irq(void) {}

void
picorb_hal_disable_irq(void) {}

#else

static esp_timer_handle_t tick_timer = NULL;

uint64_t
Machine_uptime_us(void) {
  return (uint64_t)esp_timer_get_time();
}

static void
handle_tick_timer(void *argument) {
  (void)argument;

  mrb_state *mrb = ticking_mrb;

  if (mrb != NULL)
    mrb_tick(mrb);
}

void
advance_pino_ticks(void) {}

void
picorb_hal_init(mrb_state *mrb) {
  ticking_mrb = mrb;

  if (tick_timer != NULL)
    return;

  esp_timer_create_args_t timer_arguments = {
    .callback = handle_tick_timer,
    .arg = NULL,
    .dispatch_method = ESP_TIMER_TASK,
    .name = "pino_tick",
  };

  if (esp_timer_create(&timer_arguments, &tick_timer) != ESP_OK) {
    tick_timer = NULL;
    return;
  }

  esp_timer_start_periodic(tick_timer, TICK_MICROSECONDS);
}

void
picorb_hal_idle_cpu(mrb_state *mrb) {
  (void)mrb;
  vTaskDelay(1);
}

void
picorb_hal_sleep_us(mrb_state *mrb, mrb_int usec) {
  (void)mrb;
  ets_delay_us((uint32_t)usec);
}

void
Machine_delay_ms(uint32_t ms) {
  vTaskDelay(pdMS_TO_TICKS(ms));
}

void
Machine_busy_wait_ms(uint32_t ms) {
  ets_delay_us(ms * MICROSECONDS_PER_MILLISECOND);
}

void
Machine_busy_wait_us(uint32_t us) {
  ets_delay_us(us);
}

void
picorb_hal_enable_irq(void) {
  portENABLE_INTERRUPTS();
}

void
picorb_hal_disable_irq(void) {
  portDISABLE_INTERRUPTS();
}

#endif

void
picorb_hal_final(mrb_state *mrb) {
  (void)mrb;
  ticking_mrb = NULL;
}

int
picorb_hal_write(int fd, const void *buf, int nbytes) {
  (void)fd;
  write_pino_console((const char *)buf, nbytes);
  return nbytes;
}

int
picorb_hal_flush(int fd) {
  (void)fd;
  return 0;
}

int
picorb_hal_read_available(void) {
  return 0;
}

int
picorb_hal_getchar(void) {
  return HAL_GETCHAR_NODATA;
}

bool
picorb_hal_stdin_push(uint8_t ch) {
  (void)ch;
  return false;
}

void
picorb_hal_abort(const char *s) {
  if (s)
    picorb_hal_write(1, s, (int)strlen(s));

  abort();
}

machine_sleep_result_t
Machine_sleep_timer(bool deep, uint32_t ms) {
  (void)deep;
  (void)ms;
  return MACHINE_SLEEP_EUNSUPPORTED;
}

machine_sleep_result_t
Machine_sleep_gpio(bool deep, int pin, bool edge, bool high) {
  (void)deep;
  (void)pin;
  (void)edge;
  (void)high;
  return MACHINE_SLEEP_EUNSUPPORTED;
}

bool
Machine_get_unique_id(char *id_str) {
  strcpy(id_str, "PINO");
  return true;
}

void
Machine_tud_task(void) {}

bool
Machine_tud_mounted_q(void) {
  return false;
}

bool
Machine_bootsel_pressed_q(void) {
  return false;
}

uint32_t
Machine_stack_usage(void) {
  return 0;
}

bool
Machine_set_hwclock(const struct timespec *ts) {
  (void)ts;
  return false;
}

bool
Machine_get_hwclock(struct timespec *ts) {
  uint64_t us = Machine_uptime_us();

  ts->tv_sec = (time_t)(us / 1000000);
  ts->tv_nsec = (long)(us % 1000000) * 1000;
  return true;
}

void
Machine_exit(int status) {
  (void)status;
}

void
Machine_reboot(void) {}
