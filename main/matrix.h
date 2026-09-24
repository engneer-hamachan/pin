#pragma once

typedef struct {
  int size;
  double *values;
  double *work_values;
} Matrix;

int matrix_count_buffer_elements(int size);
void matrix_attach_buffer(Matrix *matrix, int size, double *buffer);
void matrix_clear(Matrix *matrix);
void matrix_add(Matrix *matrix, int row, int column, double value);
void matrix_solve(Matrix *matrix);
