#ifndef EDITOR_PARSE_H
#define EDITOR_PARSE_H

#include <prism.h>

struct mrc_prism_arena_block *begin_editor_parse(void);
void end_editor_parse(struct mrc_prism_arena_block *outer_arena);

#endif
