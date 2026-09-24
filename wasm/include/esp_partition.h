#pragma once

#include <stddef.h>

typedef struct esp_partition_t esp_partition_t;

typedef enum { ESP_PARTITION_TYPE_APP } esp_partition_type_t;

typedef enum { ESP_PARTITION_SUBTYPE_APP_FACTORY } esp_partition_subtype_t;

static inline const esp_partition_t *
esp_partition_find_first(
  esp_partition_type_t type,
  esp_partition_subtype_t subtype,
  const char *label
) {
  (void)type;
  (void)subtype;
  (void)label;
  return NULL;
}
