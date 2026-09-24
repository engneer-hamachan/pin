#include "matrix.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

static double *
find_row_values(Matrix *matrix, int row) {
  return matrix->values + row * matrix->size;
}

static void
swap_rows(Matrix *matrix, int first_row, int second_row) {
  double *first_row_values = find_row_values(matrix, first_row);
  double *second_row_values = find_row_values(matrix, second_row);

  for (int j = 0; j < matrix->size; j++) {
    double value = first_row_values[j];
    first_row_values[j] = second_row_values[j];
    second_row_values[j] = value;
  }

  double work_value = matrix->work_values[first_row];
  matrix->work_values[first_row] = matrix->work_values[second_row];
  matrix->work_values[second_row] = work_value;
}

static int
find_pivot_row(Matrix *matrix, int column) {
  int pivot_row = column;
  double largest_magnitude = fabs(find_row_values(matrix, column)[column]);

  for (int i = column + 1; i < matrix->size; i++) {
    double magnitude = fabs(find_row_values(matrix, i)[column]);

    if (magnitude > largest_magnitude) {
      largest_magnitude = magnitude;
      pivot_row = i;
    }
  }

  return pivot_row;
}

static void
eliminate_below(Matrix *matrix, int column) {
  double *pivot_row_values = find_row_values(matrix, column);

  for (int i = column + 1; i < matrix->size; i++) {
    double *target_row_values = find_row_values(matrix, i);
    double factor = target_row_values[column] / pivot_row_values[column];

    if (factor != 0.0) {
      for (int j = column; j < matrix->size; j++) {
        target_row_values[j] -= factor * pivot_row_values[j];
      }

      matrix->work_values[i] -= factor * matrix->work_values[column];
    }
  }
}

int
matrix_count_buffer_elements(int size) {
  return size * size + size;
}

void
matrix_attach_buffer(Matrix *matrix, int size, double *buffer) {
  matrix->size = size;
  matrix->values = buffer;
  matrix->work_values = buffer ? buffer + size * size : NULL;

  if (buffer) {
    memset(buffer, 0, (size_t)matrix_count_buffer_elements(size) * sizeof(double));
  }
}

void
matrix_clear(Matrix *matrix) {
  if (matrix->values) {
    memset(matrix->values, 0, (size_t)(matrix->size * matrix->size) * sizeof(double));
  }
}

void
matrix_add(Matrix *matrix, int row, int column, double value) {
  find_row_values(matrix, row)[column] += value;
}

void
matrix_solve(Matrix *matrix) {
  for (int column = 0; column < matrix->size; column++) {
    int pivot_row = find_pivot_row(matrix, column);

    if (pivot_row != column) {
      swap_rows(matrix, column, pivot_row);
    }

    eliminate_below(matrix, column);
  }

  for (int i = matrix->size - 1; i >= 0; i--) {
    double *row_values = find_row_values(matrix, i);
    double sum = matrix->work_values[i];

    for (int j = i + 1; j < matrix->size; j++) {
      sum -= row_values[j] * matrix->work_values[j];
    }

    matrix->work_values[i] = sum / row_values[i];
  }
}
