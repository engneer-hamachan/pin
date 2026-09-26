#include "breadboard.h"

#include <stdio.h>
#include <string.h>

#define NO_GPIO -1
#define NO_ELEMENT -1
#define GND_TERMINAL 2
#define RUN_TERMINAL 29
#define ADC_VREF_TERMINAL 34
#define THREE_V_THREE_TERMINAL 35
#define THREE_V_THREE_EN_TERMINAL 36
#define VSYS_TERMINAL 38
#define VBUS_TERMINAL 39
#define POWER_ON_VOLTAGE 1.8
#define POWER_PULL_DOWN_RESISTANCE 100000.0
#define VBUS_SENSE_THRESHOLD_VOLTAGE 1.8
#define VSYS_DIODE_FORWARD_VOLTAGE 0.3
#define VSYS_DIODE_RESISTANCE 0.5
#define THREE_V_THREE_VOLTAGE 3.3
#define THREE_V_THREE_RESISTANCE 1.0
#define THREE_V_THREE_EN_PULL_UP_RESISTANCE 100000.0
#define RUN_PULL_UP_RESISTANCE 50000.0
#define ADC_VREF_FILTER_RESISTANCE 200.0
#define GPIO_OUTPUT_RESISTANCE 50.0
#define GPIO_PULL_RESISTANCE 50000.0
#define GPIO_HIGH_THRESHOLD_VOLTAGE (THREE_V_THREE_VOLTAGE / 2)

typedef struct {
  const char *name;
  int gpio;
} PinoPin;

typedef struct {
  int direction;
  int pull;
  int level;
  double voltage;
} PinoGpio;

static const PinoPin PINO_PINS[PINO_PIN_COUNT] = {
  {"GP0", 0},       {"GP1", 1},         {"GND", NO_GPIO},
  {"GP2", 2},       {"GP3", 3},         {"GP4", 4},
  {"GP5", 5},       {"GND", NO_GPIO},   {"GP6", 6},
  {"GP7", 7},       {"GP8", 8},         {"GP9", 9},
  {"GND", NO_GPIO}, {"GP10", 10},       {"GP11", 11},
  {"GP12", 12},     {"GP13", 13},       {"GND", NO_GPIO},
  {"GP14", 14},     {"GP15", 15},       {"GP16", 16},
  {"GP17", 17},     {"GND", NO_GPIO},   {"GP18", 18},
  {"GP19", 19},     {"GP20", 20},       {"GP21", 21},
  {"GND", NO_GPIO}, {"GP22", 22},       {"RUN", NO_GPIO},
  {"GP26", 26},     {"GP27", 27},       {"AGND", NO_GPIO},
  {"GP28", 28},     {"ADC_VREF", NO_GPIO}, {"3V3(OUT)", NO_GPIO},
  {"3V3_EN", NO_GPIO}, {"GND", NO_GPIO}, {"VSYS", NO_GPIO},
  {"VBUS", NO_GPIO},
};

static Breadboard *pino_board = NULL;
static PinoGpio gpios[PINO_GPIO_COUNT];
static int gpio_element_indices[PINO_GPIO_COUNT];
static int gpio_terminal_indices[PINO_GPIO_COUNT];
static int three_v_three_element_index = NO_ELEMENT;
static bool pino_powered = false;
static double vbus_voltage = 0.0;

const char *
find_pino_pin_name(int terminal_index) {
  return PINO_PINS[terminal_index].name;
}

static bool
is_ground_pin(int terminal_index) {
  const char *name = PINO_PINS[terminal_index].name;

  return strcmp(name, "GND") == 0 || strcmp(name, "AGND") == 0;
}

static int
add_pino_element(Breadboard *board, CircuitElement element) {
  int element_index = circuit_add(&board->circuit, element);

  if (element_index < 0)
    snprintf(board->message, sizeof(board->message), "No memory");

  return element_index;
}

static int
add_pino_source(
  Breadboard *board,
  int positive_node,
  int negative_node,
  double voltage,
  double resistance
) {

  CircuitElement element = {0};

  element.kind = CIRCUIT_SOURCE;
  element.pins[0] = positive_node;
  element.pins[1] = negative_node;
  element.voltage = voltage;
  element.resistance = resistance;
  return add_pino_element(board, element);
}

static void
add_pino_resistor(
  Breadboard *board,
  int first_node,
  int second_node,
  double resistance
) {

  CircuitElement element = {0};

  element.kind = CIRCUIT_RESISTOR;
  element.pins[0] = first_node;
  element.pins[1] = second_node;
  element.resistance = resistance;
  add_pino_element(board, element);
}

static void
add_pino_diode(
  Breadboard *board,
  int anode_node,
  int cathode_node,
  double forward_voltage,
  double resistance
) {

  CircuitElement element = {0};

  element.kind = CIRCUIT_DIODE;
  element.pins[0] = anode_node;
  element.pins[1] = cathode_node;
  element.voltage = forward_voltage;
  element.resistance = resistance;
  add_pino_element(board, element);
}

static void
connect_ground_pins(Breadboard *board, const int16_t *nodes) {
  for (int i = 0; i < PINO_PIN_COUNT; i++) {
    if (i != GND_TERMINAL && is_ground_pin(i))
      circuit_connect(&board->circuit, nodes[i], nodes[GND_TERMINAL]);
  }
}

static void
add_power_elements(Breadboard *board, const int16_t *nodes) {
  int ground = nodes[GND_TERMINAL];

  add_pino_resistor(
    board,
    nodes[VBUS_TERMINAL],
    ground,
    POWER_PULL_DOWN_RESISTANCE
  );
  add_pino_resistor(
    board,
    nodes[VSYS_TERMINAL],
    ground,
    POWER_PULL_DOWN_RESISTANCE
  );
  add_pino_diode(
    board,
    nodes[VBUS_TERMINAL],
    nodes[VSYS_TERMINAL],
    VSYS_DIODE_FORWARD_VOLTAGE,
    VSYS_DIODE_RESISTANCE
  );
  three_v_three_element_index = add_pino_source(
    board,
    nodes[THREE_V_THREE_TERMINAL],
    ground,
    THREE_V_THREE_VOLTAGE,
    THREE_V_THREE_RESISTANCE
  );
  add_pino_resistor(
    board,
    nodes[THREE_V_THREE_EN_TERMINAL],
    nodes[VSYS_TERMINAL],
    THREE_V_THREE_EN_PULL_UP_RESISTANCE
  );
  add_pino_resistor(
    board,
    nodes[RUN_TERMINAL],
    nodes[THREE_V_THREE_TERMINAL],
    RUN_PULL_UP_RESISTANCE
  );
  add_pino_resistor(
    board,
    nodes[ADC_VREF_TERMINAL],
    nodes[THREE_V_THREE_TERMINAL],
    ADC_VREF_FILTER_RESISTANCE
  );
}

void
build_pino_elements(Breadboard *board, Part *part) {
  const int16_t *nodes = part->terminal_node_indices;

  pino_board = board;

  for (int i = 0; i < PINO_GPIO_COUNT; i++) {
    gpio_element_indices[i] = NO_ELEMENT;
    gpio_terminal_indices[i] = NO_GPIO;
  }

  connect_ground_pins(board, nodes);
  add_power_elements(board, nodes);

  for (int i = 0; i < PINO_PIN_COUNT; i++) {
    int gpio = PINO_PINS[i].gpio;

    if (gpio == NO_GPIO)
      continue;

    gpio_terminal_indices[gpio] = i;
    gpio_element_indices[gpio] =
      add_pino_source(board, nodes[i], nodes[GND_TERMINAL], 0.0, 1.0);
  }
}

static void
configure_gpio_element(CircuitElement *element, const PinoGpio *gpio) {
  element->enabled = pino_powered;

  if (!pino_powered)
    return;

  if (gpio->direction == PINO_DIRECTION_OUT) {
    element->voltage = gpio->level ? THREE_V_THREE_VOLTAGE : 0.0;
    element->resistance = GPIO_OUTPUT_RESISTANCE;
    return;
  }

  element->resistance = GPIO_PULL_RESISTANCE;

  switch (gpio->pull) {
  case PINO_PULL_UP:
    element->voltage = THREE_V_THREE_VOLTAGE;
    break;
  case PINO_PULL_DOWN:
    element->voltage = 0.0;
    break;
  default:
    element->enabled = false;
    break;
  }
}

void
configure_pino_elements(Breadboard *board, const Part *part) {
  (void)part;

  if (three_v_three_element_index != NO_ELEMENT)
    board->circuit.elements[three_v_three_element_index].enabled = pino_powered;

  for (int i = 0; i < PINO_GPIO_COUNT; i++) {
    if (gpio_element_indices[i] == NO_ELEMENT)
      continue;

    configure_gpio_element(
      &board->circuit.elements[gpio_element_indices[i]],
      &gpios[i]
    );
  }
}

void
read_pino_solution(Breadboard *board, const Part *part) {
  const int16_t *nodes = part->terminal_node_indices;
  bool powered = circuit_voltage(
                   &board->circuit,
                   nodes[VSYS_TERMINAL],
                   nodes[GND_TERMINAL]
                 ) >= POWER_ON_VOLTAGE;

  if (powered != pino_powered) {
    pino_powered = powered;
    board->circuit_dirty = true;
    board->needs_redraw = true;
  }

  vbus_voltage =
    circuit_voltage(&board->circuit, nodes[VBUS_TERMINAL], nodes[GND_TERMINAL]);

  for (int i = 0; i < PINO_GPIO_COUNT; i++) {
    if (gpio_terminal_indices[i] == NO_GPIO)
      continue;

    gpios[i].voltage = circuit_voltage(
      &board->circuit,
      nodes[gpio_terminal_indices[i]],
      nodes[GND_TERMINAL]
    );
  }
}

static bool
is_gpio_valid(int gpio) {
  return gpio >= 0 && gpio < PINO_GPIO_COUNT;
}

static void
mark_gpio_changed(void) {
  if (pino_board == NULL)
    return;

  pino_board->circuit_dirty = true;
  pino_board->needs_redraw = true;
}

void
reset_pino_gpios(void) {
  for (int i = 0; i < PINO_GPIO_COUNT; i++)
    init_pino_gpio(i);
}

bool
is_pino_powered(void) {
  return pino_powered;
}

void
reset_pino_power(void) {
  pino_powered = false;
  vbus_voltage = 0.0;
}

bool
is_pino_led_lit(void) {
  const PinoGpio *gpio = &gpios[PINO_LED_GPIO];

  return pino_powered && gpio->direction == PINO_DIRECTION_OUT && gpio->level;
}

void
init_pino_gpio(int gpio) {
  if (!is_gpio_valid(gpio))
    return;

  gpios[gpio].direction = PINO_DIRECTION_NONE;
  gpios[gpio].pull = PINO_PULL_NONE;
  gpios[gpio].level = 0;
  mark_gpio_changed();
}

void
set_pino_gpio_direction(int gpio, int direction) {
  if (!is_gpio_valid(gpio))
    return;

  gpios[gpio].direction = direction;
  mark_gpio_changed();
}

void
set_pino_gpio_pull(int gpio, int pull) {
  if (!is_gpio_valid(gpio))
    return;

  gpios[gpio].pull = pull;
  mark_gpio_changed();
}

void
write_pino_gpio(int gpio, int level) {
  if (!is_gpio_valid(gpio))
    return;

  gpios[gpio].level = level ? 1 : 0;
  mark_gpio_changed();
}

int
read_pino_gpio(int gpio) {
  if (!is_gpio_valid(gpio))
    return 0;

  if (gpio == PINO_VBUS_SENSE_GPIO)
    return vbus_voltage >= VBUS_SENSE_THRESHOLD_VOLTAGE;

  if (gpio_terminal_indices[gpio] == NO_GPIO)
    return gpios[gpio].direction == PINO_DIRECTION_OUT ? gpios[gpio].level : 0;

  if (pino_board != NULL && pino_board->circuit_dirty)
    solve_circuit(pino_board);

  return gpios[gpio].voltage > GPIO_HIGH_THRESHOLD_VOLTAGE;
}
