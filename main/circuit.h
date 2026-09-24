#pragma once
#include "matrix.h"
#include <stdbool.h>
#include <stddef.h>

typedef void *(*CircuitAllocator)(void *context, void *buffer, size_t bytes);
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
  double *voltages;
  int maximum_nodes, node_count;
  int *parents, *node_indices;
  CircuitElement *elements;
  int count, capacity;
  bool topology_dirty;
  CircuitAllocator allocate;
  void *context;
} Circuit;

bool circuit_init(Circuit *circuit, int maximum_nodes, CircuitAllocator allocate, void *context);
void circuit_release(Circuit *circuit);
void circuit_clear(Circuit *circuit);
void circuit_connect(Circuit *circuit, int first, int second);
int circuit_add(Circuit *circuit, CircuitElement element);
bool circuit_step(Circuit *circuit, double dt, int iterations);
double circuit_voltage(Circuit *circuit, int positive, int negative);
