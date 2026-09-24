#include "esp_random.h"
#include "esp_system.h"
#include "freertos/task.h"

#include <emscripten.h>

#define RESTART_WAIT_MS 1000

void app_main(void);

EM_JS(uint32_t, esp_random, (void), {
  return crypto.getRandomValues(new Uint32Array(1))[0];
});

void
vTaskDelay(TickType_t ticks) {
  emscripten_sleep(ticks);
}

void
esp_restart(void) {
  emscripten_run_script("location.reload()");

  for (;;)
    emscripten_sleep(RESTART_WAIT_MS);
}

int
main(void) {
  app_main();
  return 0;
}
