#include "breadboard.h"
#include "canvas.h"
#include "game.h"
#include "keyboard.h"

#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdlib.h>

#define FRAME_INTERVAL_MS 50
#define INITIAL_CURSOR_ROW 2

static Breadboard board;

static void *
resize_circuit_buffer(void *context, void *buffer, size_t byte_count) {
  (void)context;

  if (byte_count == 0) {
    free(buffer);
    return NULL;
  }

  return realloc(buffer, byte_count);
}

static void
init_breadboard(void) {
  if (!circuit_init(&board.circuit, NET_COUNT, resize_circuit_buffer, NULL))
    abort();

  board.cursor_row = INITIAL_CURSOR_ROW;
  board.needs_redraw = true;
  cancel_placement(&board);
  update_circuit_structure(&board);
}

static void
run_breadboard(void) {
  while (!board.quit) {
    int key = keyboard_read_key();

    if (key != KEY_NONE)
      handle_key(&board, key);

    step_pino_program();
    step_simulation(&board);

    if (board.needs_redraw || is_animation_running(&board))
      draw_screen(&board);

    board.needs_redraw = false;
    board.frame_count++;
    vTaskDelay(pdMS_TO_TICKS(FRAME_INTERVAL_MS));
  }
}

static void
restart_into_factory_app(void) {
  const esp_partition_t *factory_partition = esp_partition_find_first(
    ESP_PARTITION_TYPE_APP,
    ESP_PARTITION_SUBTYPE_APP_FACTORY,
    NULL
  );

  if (factory_partition != NULL &&
      factory_partition != esp_ota_get_running_partition())
    esp_ota_set_boot_partition(factory_partition);

  esp_restart();
}

void
app_main(void) {
  canvas_begin();
  keyboard_begin();
  init_breadboard();

  for (;;) {
    TitleChoice choice = run_title_screen();

    if (choice == TITLE_CHOICE_GAME) {
      run_game(&board);
      continue;
    }

    if (choice == TITLE_CHOICE_SIMULATOR) {
      board.quit = false;
      run_breadboard();
      continue;
    }

    break;
  }

  restart_into_factory_app();
}
