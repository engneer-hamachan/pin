#ifndef EDITOR_HOST_H
#define EDITOR_HOST_H

void compute_editor_grid(int *columns, int *rows);
void mark_editor_canvas_changed(void);
int editor_wait_byte(void);

// sequence must hold 2 bytes; returns how many following bytes were read (0-2).
int read_escape_sequence(char *sequence);

#endif
