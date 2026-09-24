#include "breadboard.h"

#include <errno.h>
#include <sys/stat.h>

const char *const SAVE_FILE_NAMES[SAVE_SLOT_COUNT] = {
  "pin.txt",
  "pin2.txt",
  "pin3.txt",
  "pin4.txt",
};

// SD_MOUNT_PATH is a directory in Emscripten's in-memory file system.
bool
mount_sd(void) {
  return mkdir(SD_MOUNT_PATH, 0775) == 0 || errno == EEXIST;
}

void
unmount_sd(void) {}

bool
save_board(const Breadboard *board, int slot_index) {
  (void)board;
  (void)slot_index;
  return false;
}

bool
load_board(Breadboard *board, int slot_index) {
  (void)board;
  (void)slot_index;
  return false;
}
