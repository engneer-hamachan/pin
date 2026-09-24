#include "game.h"

#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>

#define HISCORE_FILE_PATH SAVE_DIRECTORY_PATH "/hiscore.txt"

static int
read_hiscore(void) {
  FILE *file = fopen(HISCORE_FILE_PATH, "r");

  if (file == NULL)
    return 0;

  int score;

  if (fscanf(file, "%d", &score) != 1 || score < 0)
    score = 0;

  fclose(file);
  return score;
}

static bool
write_hiscore(int score) {
  if (mkdir(SAVE_DIRECTORY_PATH, 0775) != 0 && errno != EEXIST)
    return false;

  FILE *file = fopen(HISCORE_FILE_PATH, "w");

  if (file == NULL)
    return false;

  fprintf(file, "%d\n", score);
  return fclose(file) == 0;
}

int
load_hiscore(void) {
  if (!mount_sd())
    return 0;

  int score = read_hiscore();

  unmount_sd();
  return score;
}

bool
save_hiscore(int score) {
  if (!mount_sd())
    return false;

  bool saved = write_hiscore(score);

  unmount_sd();
  return saved;
}
