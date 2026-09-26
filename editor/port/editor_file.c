#include "port/editor_file.h"

#include "breadboard.h"
#include "core/text/string.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#define EDIT_PATH_SIZE 128

static int
copy_edit_path(char *destination, const char *path, int path_byte_length) {
  if (path_byte_length <= 0 || path_byte_length >= EDIT_PATH_SIZE)
    return 0;

  memcpy(destination, path, (size_t)path_byte_length);
  destination[path_byte_length] = '\0';
  return 1;
}

int
load_edit_file(const char *path, int path_byte_length, VimString *content) {
  char path_buffer[EDIT_PATH_SIZE];

  if (!copy_edit_path(path_buffer, path, path_byte_length))
    return 0;

  if (!mount_sd())
    return 0;

  FILE *file = fopen(path_buffer, "rb");

  if (file == NULL) {
    unmount_sd();
    return 0;
  }

  char read_buffer[256];
  size_t read_size;

  while ((read_size = fread(read_buffer, 1, sizeof(read_buffer), file)) > 0) {
    vim_string_append(content, read_buffer, (int)read_size);
  }

  fclose(file);
  unmount_sd();

  return 1;
}

int
save_edit_file(Vim *core) {
  char path_buffer[EDIT_PATH_SIZE];

  if (!copy_edit_path(
        path_buffer,
        core->filepath.bytes,
        core->filepath.byte_length
      ))
    return 0;

  if (!mount_sd())
    return 0;

  if (mkdir(SAVE_DIRECTORY_PATH, 0775) != 0 && errno != EEXIST) {
    unmount_sd();
    return 0;
  }

  VimString content;
  vim_string_init(&content);
  vim_write_content(core, &content);

  int saved = 0;

  FILE *file = fopen(path_buffer, "wb");

  if (file != NULL) {
    size_t written = 0;

    if (content.byte_length > 0)
      written = fwrite(content.bytes, 1, (size_t)content.byte_length, file);

    saved = written == (size_t)content.byte_length;

    if (fclose(file) != 0)
      saved = 0;
  }

  vim_string_free(&content);
  unmount_sd();

  return saved;
}
