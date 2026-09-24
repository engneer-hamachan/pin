#include "game.h"

#include <math.h>

#define FLOW_CURRENT 0.0005
#define NET_EDGE_CAPACITY (PART_CAPACITY * CONTACT_PAIR_CAPACITY)

typedef struct {
  int first_net;
  int second_net;
  int part_index;
  bool kept;
} NetEdge;

static NetEdge net_edges[NET_EDGE_CAPACITY];
static int net_parents[NET_COUNT];
static bool removal_marks[PART_CAPACITY];

static bool
has_lit_level(const Part *part) {
  for (int i = 0; i < part->level_count; i++) {
    if (part->levels[i] > 0)
      return true;
  }

  return false;
}

static bool
is_output_active(const Part *part) {
  if (part->burned)
    return false;

  switch (part->kind) {
  case PART_KIND_LED:
  case PART_KIND_RGB_LED:
  case PART_KIND_SEVEN_SEGMENT:
  case PART_KIND_BUZZER:
    return has_lit_level(part);
  case PART_KIND_MOTOR:
    return compute_motor_phase_step(part) != 0;
  default:
    return false;
  }
}

bool
has_active_output(const Breadboard *board) {
  for (int i = 0; i < board->part_count; i++) {
    if (is_output_active(&board->parts[i]))
      return true;
  }

  return false;
}

bool
has_burned_part(const Breadboard *board) {
  for (int i = 0; i < board->part_count; i++) {
    if (board->parts[i].burned)
      return true;
  }

  return false;
}

static bool
is_element_flowing(const CircuitElement *element) {
  return fabs(element->current) > FLOW_CURRENT ||
         fabs(element->control_current) > FLOW_CURRENT;
}

static int
count_element_pins(const CircuitElement *element) {
  return element->kind == CIRCUIT_NPN ? 3 : 2;
}

static bool
is_part_flowing(const Breadboard *board, const Part *part) {
  for (int i = 0; i < part->element_count; i++) {
    if (is_element_flowing(&board->circuit.elements[part->elements[i]]))
      return true;
  }

  return false;
}

static void
mark_flowing_nets(const Breadboard *board, bool *flowing_nets) {
  for (int i = 0; i < NET_COUNT; i++)
    flowing_nets[i] = false;

  for (int i = 0; i < board->circuit.count; i++) {
    const CircuitElement *element = &board->circuit.elements[i];

    if (!is_element_flowing(element))
      continue;

    for (int j = 0; j < count_element_pins(element); j++)
      flowing_nets[element->pins[j]] = true;
  }
}

static int
collect_net_edges(const Breadboard *board) {
  int edge_count = 0;

  for (int i = 0; i < board->part_count; i++) {
    const Part *part = &board->parts[i];
    int contact_pairs[CONTACT_PAIR_CAPACITY][2];
    int contact_pair_count =
      list_contact_terminal_pairs(part->kind, contact_pairs);

    for (int j = 0; j < contact_pair_count; j++) {
      NetEdge *edge = &net_edges[edge_count++];

      edge->first_net = part->terminal_node_indices[contact_pairs[j][0]];
      edge->second_net = part->terminal_node_indices[contact_pairs[j][1]];
      edge->part_index = i;
      edge->kept = true;
    }
  }

  return edge_count;
}

// Drops edges that end on a net with no flowing element and no other edge.
static void
drop_dead_end_edges(int edge_count, const bool *flowing_nets) {
  int degrees[NET_COUNT] = {0};

  for (int i = 0; i < edge_count; i++) {
    degrees[net_edges[i].first_net]++;
    degrees[net_edges[i].second_net]++;
  }

  bool dropped = true;

  while (dropped) {
    dropped = false;

    for (int i = 0; i < edge_count; i++) {
      NetEdge *edge = &net_edges[i];

      if (!edge->kept)
        continue;

      bool first_dead =
        !flowing_nets[edge->first_net] && degrees[edge->first_net] == 1;
      bool second_dead =
        !flowing_nets[edge->second_net] && degrees[edge->second_net] == 1;

      if (!first_dead && !second_dead)
        continue;

      edge->kept = false;
      degrees[edge->first_net]--;
      degrees[edge->second_net]--;
      dropped = true;
    }
  }
}

static int
find_net_root(int net) {
  while (net_parents[net] != net)
    net = net_parents[net];

  return net;
}

// Drops edges whose group of joined nets has no flowing element.
static void
drop_floating_edges(int edge_count, const bool *flowing_nets) {
  bool flowing_roots[NET_COUNT];

  for (int i = 0; i < NET_COUNT; i++) {
    net_parents[i] = i;
    flowing_roots[i] = false;
  }

  for (int i = 0; i < edge_count; i++) {
    if (net_edges[i].kept)
      net_parents[find_net_root(net_edges[i].first_net)] =
        find_net_root(net_edges[i].second_net);
  }

  for (int i = 0; i < NET_COUNT; i++) {
    if (flowing_nets[i])
      flowing_roots[find_net_root(i)] = true;
  }

  for (int i = 0; i < edge_count; i++) {
    if (!flowing_roots[find_net_root(net_edges[i].first_net)])
      net_edges[i].kept = false;
  }
}

void
mark_current_path_parts(const Breadboard *board) {
  bool flowing_nets[NET_COUNT];

  mark_flowing_nets(board, flowing_nets);

  int edge_count = collect_net_edges(board);

  drop_dead_end_edges(edge_count, flowing_nets);
  drop_floating_edges(edge_count, flowing_nets);

  for (int i = 0; i < board->part_count; i++)
    removal_marks[i] = is_part_flowing(board, &board->parts[i]);

  for (int i = 0; i < edge_count; i++) {
    if (net_edges[i].kept)
      removal_marks[net_edges[i].part_index] = true;
  }

  for (int i = 0; i < board->part_count; i++) {
    if (board->parts[i].kind == PART_KIND_BATTERY)
      removal_marks[i] = false;
  }
}

bool
is_part_marked(int part_index) {
  return removal_marks[part_index];
}

int
remove_marked_parts(Breadboard *board) {
  int removed_count = 0;

  for (int i = board->part_count - 1; i >= 0; i--) {
    if (!removal_marks[i])
      continue;

    remove_part(board, &board->parts[i]);
    removed_count++;
  }

  return removed_count;
}

static bool
has_two_free_holes(Breadboard *board, bool rails_allowed) {
  int free_count = 0;

  for (int i = 0; i < HOLE_COUNT; i++) {
    if (find_part_at(board, i))
      continue;

    if (!rails_allowed && is_rail_hole(i))
      continue;

    free_count++;

    if (free_count == 2)
      return true;
  }

  return false;
}

bool
touches_rail(const int *hole_indices, int hole_count) {
  for (int i = 0; i < hole_count; i++) {
    if (is_rail_hole(hole_indices[i]))
      return true;
  }

  return false;
}

bool
can_place_anywhere(Breadboard *board, PartKind kind) {
  if (!has_footprint(kind))
    return has_two_free_holes(board, kind == PART_KIND_WIRE);

  int hole_indices[PART_TERMINAL_CAPACITY];

  for (int row = 0; row < BOARD_ROW_COUNT; row++) {
    for (int column = 0; column < BOARD_COLUMN_COUNT; column++) {
      if (!compute_footprint_hole_indices(kind, row, column, hole_indices))
        continue;

      if (touches_rail(hole_indices, count_terminals(kind)))
        continue;

      if (are_holes_free(board, hole_indices, count_terminals(kind)))
        return true;
    }
  }

  return false;
}
