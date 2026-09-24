#include "breadboard.h"

#include "driver/gpio.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>

#define SD_PIN_SCK 40
#define SD_PIN_MISO 39
#define SD_PIN_MOSI 14
#define SD_PIN_CS 12
#define SD_SPI_HOST SPI2_HOST
#define SD_FREQUENCY_KHZ 20000
#define SAVE_PATH_SIZE 64
#define SAVE_LINE_SIZE 128
#define LAST_PART_KIND PART_KIND_CAPACITOR

const char *const SAVE_FILE_NAMES[SAVE_SLOT_COUNT] = {
  "pin.txt",
  "pin2.txt",
  "pin3.txt",
  "pin4.txt",
};

static sdmmc_card_t *sd_card = NULL;
#if !defined(PIN_BOARD_TFT_ST7789) && !defined(PIN_BOARD_TFT_ILI9341)
static bool sd_bus_initialized = false;
#endif

bool
mount_sd(void) {
  // TFT initializes SPI2 in canvas_begin, including MISO.
#if !defined(PIN_BOARD_TFT_ST7789) && !defined(PIN_BOARD_TFT_ILI9341)
  if (!sd_bus_initialized) {
    spi_bus_config_t bus_config = {
      .mosi_io_num = SD_PIN_MOSI,
      .miso_io_num = SD_PIN_MISO,
      .sclk_io_num = SD_PIN_SCK,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz = 4000,
    };

    if (spi_bus_initialize(SD_SPI_HOST, &bus_config, SPI_DMA_CH_AUTO) !=
        ESP_OK)
      return false;

    sd_bus_initialized = true;
  }
#endif

  // ESP-IDF sdspi sets no pull-ups; floating lines garble CMD8.
  gpio_set_pull_mode(SD_PIN_MISO, GPIO_PULLUP_ONLY);
  gpio_set_pull_mode(SD_PIN_MOSI, GPIO_PULLUP_ONLY);
  gpio_set_pull_mode(SD_PIN_SCK, GPIO_PULLUP_ONLY);
  gpio_set_pull_mode(SD_PIN_CS, GPIO_PULLUP_ONLY);

  sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
  slot_config.gpio_cs = SD_PIN_CS;
  slot_config.host_id = SD_SPI_HOST;

  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  host.slot = SD_SPI_HOST;
  host.max_freq_khz = SD_FREQUENCY_KHZ;

  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
    .format_if_mount_failed = false,
    .max_files = 2,
    .allocation_unit_size = 512,
  };

  if (esp_vfs_fat_sdspi_mount(
        SD_MOUNT_PATH,
        &host,
        &slot_config,
        &mount_config,
        &sd_card
      ) != ESP_OK) {
    sd_card = NULL;
    return false;
  }

  return true;
}

void
unmount_sd(void) {
  esp_vfs_fat_sdcard_unmount(SD_MOUNT_PATH, sd_card);
  sd_card = NULL;
}

static void
format_save_file_path(int slot_index, char *path, size_t path_size) {
  snprintf(
    path,
    path_size,
    "%s/%s",
    SAVE_DIRECTORY_PATH,
    SAVE_FILE_NAMES[slot_index]
  );
}

static void
write_part(FILE *file, const Part *part) {
  fprintf(
    file,
    "%d %d %d %d %d %d",
    (int)part->kind,
    part->color_index,
    part->resistance_index,
    part->knob_position,
    part->light_level,
    part->slide_position
  );

  for (int i = 0; i < part->terminal_count; i++)
    fprintf(file, " %d", part->terminal_hole_indices[i]);

  fputc('\n', file);
}

static bool
write_board(const Breadboard *board, int slot_index) {
  if (mkdir(SAVE_DIRECTORY_PATH, 0775) != 0 && errno != EEXIST)
    return false;

  char path[SAVE_PATH_SIZE];

  format_save_file_path(slot_index, path, sizeof path);

  FILE *file = fopen(path, "w");

  if (file == NULL)
    return false;

  for (int i = 0; i < board->part_count; i++)
    write_part(file, &board->parts[i]);

  return fclose(file) == 0;
}

static bool
read_hole_indices(
  const char *text,
  int terminal_count,
  int *hole_indices
) {

  for (int i = 0; i < terminal_count; i++) {
    int length;

    if (sscanf(text, "%d%n", &hole_indices[i], &length) != 1)
      return false;

    if (hole_indices[i] < 0 || hole_indices[i] >= HOLE_COUNT)
      return false;

    text += length;
  }

  return true;
}

static void
read_part(Breadboard *board, const char *line) {
  int kind;
  int color_index;
  int resistance_index;
  int knob_position;
  int light_level;
  int slide_position;
  int length;

  if (sscanf(
        line,
        "%d %d %d %d %d %d%n",
        &kind,
        &color_index,
        &resistance_index,
        &knob_position,
        &light_level,
        &slide_position,
        &length
      ) != 6)
    return;

  if (kind < 0 || kind > LAST_PART_KIND)
    return;

  int terminal_count = count_terminals((PartKind)kind);
  int hole_indices[PART_TERMINAL_CAPACITY];

  if (!read_hole_indices(line + length, terminal_count, hole_indices))
    return;

  if (!are_holes_free(board, hole_indices, terminal_count))
    return;

  if (board->part_count == PART_CAPACITY)
    return;

  add_part(board, (PartKind)kind, hole_indices, color_index);

  Part *part = &board->parts[board->part_count - 1];

  part->color_index = color_index;
  part->resistance_index =
    clamp_integer(resistance_index, 0, RESISTOR_VALUE_COUNT - 1);
  part->knob_position = clamp_integer(knob_position, 0, KNOB_POSITION_MAXIMUM);
  part->light_level = clamp_integer(light_level, 0, LIGHT_LEVEL_MAXIMUM);
  part->slide_position = clamp_integer(slide_position, 0, 1);
}

static bool
read_board(Breadboard *board, int slot_index) {
  char path[SAVE_PATH_SIZE];

  format_save_file_path(slot_index, path, sizeof path);

  FILE *file = fopen(path, "r");

  if (file == NULL)
    return false;

  char line[SAVE_LINE_SIZE];

  remove_all_parts(board);

  while (fgets(line, sizeof line, file))
    read_part(board, line);

  fclose(file);
  board->circuit_dirty = true;
  return true;
}

bool
save_board(const Breadboard *board, int slot_index) {
  if (!mount_sd())
    return false;

  bool saved = write_board(board, slot_index);

  unmount_sd();
  return saved;
}

bool
load_board(Breadboard *board, int slot_index) {
  if (!mount_sd())
    return false;

  bool loaded = read_board(board, slot_index);

  unmount_sd();
  return loaded;
}
