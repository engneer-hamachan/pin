#include "circuit.h"
#include <string.h>

static int root(Circuit *circuit, int node) {
  while (circuit->parents[node] != node) node = circuit->parents[node];
  return node;
}

void circuit_clear(Circuit *circuit) {
  circuit->count = 0;
  circuit->node_count = 0;
  circuit->topology_dirty = true;
  for (int i = 0; i < CIRCUIT_NODE_CAPACITY; i++) {
    circuit->parents[i] = i;
    circuit->node_indices[i] = -1;
  }
  matrix_attach_buffer(&circuit->matrix, 0, NULL);
  circuit->voltages = NULL;
}

void circuit_connect(Circuit *circuit, int first, int second) {
  circuit->parents[root(circuit, first)] = root(circuit, second);
  circuit->topology_dirty = true;
}

int circuit_add(Circuit *circuit, CircuitElement element) {
  if (circuit->count == CIRCUIT_ELEMENT_CAPACITY) return -1;
  element.enabled = true;
  circuit->elements[circuit->count] = element;
  circuit->topology_dirty = true;
  return circuit->count++;
}

static void compile_nodes(Circuit *circuit) {
  for (int i = 0; i < CIRCUIT_NODE_CAPACITY; i++) circuit->node_indices[i] = -1;
  int count = 0;
  for (int i = 0; i < circuit->count; i++) {
    CircuitElement *element = &circuit->elements[i];
    int pins = element->kind == CIRCUIT_NPN ? 3 : 2;
    for (int j = 0; j < pins; j++) {
      int node = root(circuit, element->pins[j]);
      if (circuit->node_indices[node] < 0) circuit->node_indices[node] = count++;
    }
  }
  if (count != circuit->matrix.size) {
    double *buffer = count ? circuit->matrix_buffer : NULL;
    matrix_attach_buffer(&circuit->matrix, count, buffer);
    circuit->voltages = count ? buffer + (size_t)count * (count + 1) : NULL;
    if (count) memset(circuit->voltages, 0, (size_t)count * sizeof(double));
  }
  circuit->node_count = count;
  circuit->topology_dirty = false;
}

static int node_index(Circuit *circuit, int pin) {
  return circuit->node_indices[root(circuit, pin)];
}

double circuit_voltage(Circuit *circuit, int positive, int negative) {
  int first = node_index(circuit, positive), second = node_index(circuit, negative);
  double a = first < 0 || !circuit->voltages ? 0.0 : circuit->voltages[first];
  double b = second < 0 || !circuit->voltages ? 0.0 : circuit->voltages[second];
  return a - b;
}

static void conductance(Circuit *circuit, int a, int b, double g) {
  matrix_add(&circuit->matrix, a, a, g);
  matrix_add(&circuit->matrix, b, b, g);
  matrix_add(&circuit->matrix, a, b, -g);
  matrix_add(&circuit->matrix, b, a, -g);
}

static void source(Circuit *circuit, int a, int b, double voltage, double resistance) {
  double g = 1.0 / resistance;
  conductance(circuit, a, b, g);
  circuit->matrix.work_values[a] += g * voltage;
  circuit->matrix.work_values[b] -= g * voltage;
}

static void stamp(Circuit *circuit, CircuitElement *element, double dt) {
  if (!element->enabled) return;
  int a = node_index(circuit, element->pins[0]);
  int b = node_index(circuit, element->pins[1]);
  switch (element->kind) {
  case CIRCUIT_RESISTOR: conductance(circuit, a, b, 1.0 / element->resistance); break;
  case CIRCUIT_SOURCE: source(circuit, a, b, element->voltage, element->resistance); break;
  case CIRCUIT_DIODE:
    if (element->conducting) source(circuit, a, b, element->voltage, element->resistance);
    break;
  case CIRCUIT_CAPACITOR: source(circuit, a, b, element->voltage, dt / element->value); break;
  case CIRCUIT_NPN: {
    int base = node_index(circuit, element->pins[2]);
    if (element->conducting) source(circuit, base, b, element->voltage, element->resistance);
    if (element->mode == 1) {
      double g = element->value / element->resistance;
      matrix_add(&circuit->matrix, a, base, g);
      matrix_add(&circuit->matrix, a, b, -g);
      matrix_add(&circuit->matrix, b, base, -g);
      matrix_add(&circuit->matrix, b, b, g);
      circuit->matrix.work_values[a] += g * element->voltage;
      circuit->matrix.work_values[b] -= g * element->voltage;
    } else if (element->mode == 2) {
      source(circuit, a, b, element->saturation_voltage, element->saturation_resistance);
    }
    break;
  }
  }
}

static double junction_current(Circuit *circuit, CircuitElement *element) {
  if (!element->enabled || !element->conducting) return 0.0;
  int positive = element->pins[element->kind == CIRCUIT_NPN ? 2 : 0];
  return (circuit_voltage(circuit, positive, element->pins[1]) - element->voltage) / element->resistance;
}

static double collector_current(Circuit *circuit, CircuitElement *element) {
  if (element->mode == 1) return element->value * junction_current(circuit, element);
  if (element->mode == 2)
    return (circuit_voltage(circuit, element->pins[0], element->pins[1]) - element->saturation_voltage) / element->saturation_resistance;
  return 0.0;
}

static bool update_states(Circuit *circuit) {
  bool changed = false;
  for (int i = 0; i < circuit->count; i++) {
    CircuitElement *element = &circuit->elements[i];
    if (!element->enabled || (element->kind != CIRCUIT_DIODE && element->kind != CIRCUIT_NPN)) continue;
    int positive = element->pins[element->kind == CIRCUIT_NPN ? 2 : 0];
    bool conducting = element->conducting ? junction_current(circuit, element) >= 0.0 :
      circuit_voltage(circuit, positive, element->pins[1]) > element->voltage;
    if (conducting != element->conducting) {
      element->conducting = conducting;
      changed = true;
    }
    if (element->kind == CIRCUIT_NPN) {
      int mode = 0;
      if (conducting) {
        double vce = circuit_voltage(circuit, element->pins[0], element->pins[1]);
        if (element->mode == 1) mode = vce < element->saturation_voltage ? 2 : 1;
        else if (element->mode == 2) mode = collector_current(circuit, element) > element->value * junction_current(circuit, element) ? 1 : 2;
        else mode = 1;
      }
      if (mode != element->mode) { element->mode = mode; changed = true; }
    }
  }
  return changed;
}

void circuit_step(Circuit *circuit, double dt, int iterations) {
  if (circuit->topology_dirty) compile_nodes(circuit);
  if (!circuit->node_count) return;
  for (int iteration = 0; iteration < iterations; iteration++) {
    matrix_clear(&circuit->matrix);
    for (int i = 0; i < circuit->node_count; i++) {
      matrix_add(&circuit->matrix, i, i, 1e-9);
      circuit->matrix.work_values[i] = 0.0;
    }
    for (int i = 0; i < circuit->count; i++) stamp(circuit, &circuit->elements[i], dt);
    matrix_solve(&circuit->matrix);
    memcpy(circuit->voltages, circuit->matrix.work_values, (size_t)circuit->node_count * sizeof(double));
    if (!update_states(circuit)) break;
  }
  for (int i = 0; i < circuit->count; i++) {
    CircuitElement *element = &circuit->elements[i];
    double voltage = circuit_voltage(circuit, element->pins[0], element->pins[1]);
    element->current = 0.0;
    element->control_current = 0.0;
    if (!element->enabled) continue;
    switch (element->kind) {
    case CIRCUIT_RESISTOR: element->current = voltage / element->resistance; break;
    case CIRCUIT_SOURCE: element->current = (element->voltage - voltage) / element->resistance; break;
    case CIRCUIT_DIODE: element->current = junction_current(circuit, element); break;
    case CIRCUIT_CAPACITOR:
      element->current = (voltage - element->voltage) * element->value / dt;
      element->voltage = voltage;
      break;
    case CIRCUIT_NPN:
      element->current = collector_current(circuit, element);
      element->control_current = junction_current(circuit, element);
      break;
    }
  }
}
