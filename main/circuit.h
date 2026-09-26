#pragma once
#include "matrix.h"
#include <stdbool.h>
#include <stddef.h>

#define CIRCUIT_NODE_CAPACITY 64
#define CIRCUIT_ELEMENT_CAPACITY 337

typedef enum { CIRCUIT_RESISTOR, CIRCUIT_SOURCE, CIRCUIT_DIODE, CIRCUIT_CAPACITOR, CIRCUIT_NPN } CircuitKind;
typedef struct {
  CircuitKind kind;
  int pins[3];
  double value, resistance, voltage, saturation_voltage, saturation_resistance;
  double current, control_current;
  bool enabled, conducting;
  int mode;
} CircuitElement;
typedef struct {
  Matrix matrix;
  double matrix_buffer[CIRCUIT_NODE_CAPACITY * (CIRCUIT_NODE_CAPACITY + 2)];
  double *voltages;
  int node_count;
  int parents[CIRCUIT_NODE_CAPACITY], node_indices[CIRCUIT_NODE_CAPACITY];
  CircuitElement elements[CIRCUIT_ELEMENT_CAPACITY];
  int count;
  bool topology_dirty;
} Circuit;

void circuit_clear(Circuit *circuit);
void circuit_connect(Circuit *circuit, int first, int second);
int circuit_add(Circuit *circuit, CircuitElement element);
void circuit_step(Circuit *circuit, double dt, int iterations);
double circuit_voltage(Circuit *circuit, int positive, int negative);
