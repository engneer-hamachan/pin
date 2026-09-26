#include "port/editor_parse.h"

#include <stdlib.h>

struct mrc_prism_arena_block *
begin_editor_parse(void) {
  struct mrc_prism_arena_block *outer_arena = mrc_prism_arena;

  mrc_prism_arena = NULL;
  return outer_arena;
}

void
end_editor_parse(struct mrc_prism_arena_block *outer_arena) {
  struct mrc_prism_arena_block *block = mrc_prism_arena;

  while (block != NULL) {
    struct mrc_prism_arena_block *previous_block = block->prev;

    free(block);
    block = previous_block;
  }

  mrc_prism_arena = outer_arena;
}
