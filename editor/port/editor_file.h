#ifndef EDITOR_FILE_H
#define EDITOR_FILE_H

#include "core/editor.h"

int load_edit_file(const char *path, int path_byte_length, VimString *content);
int save_edit_file(Vim *core);

#endif
