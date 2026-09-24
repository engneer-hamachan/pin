#pragma once

#include "esp_partition.h"

typedef int esp_err_t;

static inline const esp_partition_t *
esp_ota_get_running_partition(void) {
  return NULL;
}

static inline esp_err_t
esp_ota_set_boot_partition(const esp_partition_t *partition) {
  (void)partition;
  return 0;
}
