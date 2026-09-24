#pragma once

#include "breadboard.h"

#include <stdbool.h>

typedef enum {
  TITLE_CHOICE_GAME,
  TITLE_CHOICE_SIMULATOR,
  TITLE_CHOICE_QUIT
} TitleChoice;

TitleChoice run_title_screen(void);
void run_game(Breadboard *board);

bool has_active_output(const Breadboard *board);
bool has_burned_part(const Breadboard *board);
void mark_current_path_parts(const Breadboard *board);
bool is_part_marked(int part_index);
int remove_marked_parts(Breadboard *board);
bool touches_rail(const int *hole_indices, int hole_count);
bool can_place_anywhere(Breadboard *board, PartKind kind);

int load_hiscore(void);
bool save_hiscore(int score);
