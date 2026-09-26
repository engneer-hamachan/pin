#include "game.h"

#include "canvas.h"
#include "keyboard.h"
#include "theme.h"
#include "widget.h"

#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

#define GAME_FRAME_INTERVAL_MS 50
#define CLEAR_FRAMES 12
#define START_CURSOR_ROW 2
#define START_CURSOR_COLUMN 0
#define BATTERY_POSITIVE_ROW 0
#define BATTERY_NEGATIVE_ROW 1
#define BATTERY_COLUMN 1
#define CLEAR_MARKER_COLOR 0xFFD020
#define RAIL_BLOCKED_COLOR 0xFF0000
#define WIRE_WEIGHT 6
#define GAME_LED_COLOR_INDEX 0
#define PART_LIMIT 15
#define GAME_OVER_PANEL_X 30
#define GAME_OVER_PANEL_Y 30
#define GAME_OVER_PANEL_WIDTH (CANVAS_WIDTH - GAME_OVER_PANEL_X * 2)
#define GAME_OVER_LINE_HEIGHT 14
#if defined(PIN_BOARD_WASM)
#define GAME_OVER_LINE_COUNT 4
#else
#define GAME_OVER_LINE_COUNT 5
#endif

typedef enum {
  GAME_OVER_NONE,
  GAME_OVER_BURNED,
  GAME_OVER_SHORT,
  GAME_OVER_NO_ROOM,
  GAME_OVER_TOO_MANY_PARTS
} GameOverReason;

typedef struct {
  Breadboard *board;
  int score;
  int hiscore;
  int clear_frames;
  bool room_check_pending;
  bool paused;
  const PlacementEntry *paused_entry;
  bool quit;
  GameOverReason over_reason;
} Game;

static void
reset_board(Breadboard *board) {
  remove_all_parts(board);
  cancel_placement(board);
  board->cursor_row = START_CURSOR_ROW;
  board->cursor_column = START_CURSOR_COLUMN;
  board->message[0] = 0;
  board->needs_redraw = true;
}

static void
place_battery(Breadboard *board) {
  int hole_indices[2] = {
    build_hole_index(BATTERY_POSITIVE_ROW, BATTERY_COLUMN),
    build_hole_index(BATTERY_NEGATIVE_ROW, BATTERY_COLUMN),
  };

  add_part(board, PART_KIND_BATTERY, hole_indices, 0);
}

static int
find_entry_weight(const PlacementEntry *entry) {
  if (entry->kind == PART_KIND_BATTERY || entry->kind == PART_KIND_PINO)
    return 0;

  if (entry->kind == PART_KIND_LED &&
      entry->color_index != GAME_LED_COLOR_INDEX)
    return 0;

  if (entry->kind == PART_KIND_WIRE)
    return WIRE_WEIGHT;

  return 1;
}

static const PlacementEntry *
pick_next_entry(void) {
  int entry_count = count_placement_entries();
  int total_weight = 0;

  for (int i = 0; i < entry_count; i++)
    total_weight += find_entry_weight(find_placement_entry(i));

  int pick = (int)(esp_random() % (uint32_t)total_weight);

  for (int i = 0; i < entry_count; i++) {
    const PlacementEntry *entry = find_placement_entry(i);

    pick -= find_entry_weight(entry);

    if (pick < 0)
      return entry;
  }

  return find_placement_entry(0);
}

static void
deal_next_part(Game *game) {
  game->board->placing_entry = pick_next_entry();
  game->room_check_pending = true;
}

static bool
is_placement_on_rail(const Breadboard *board) {
  if (board->placing_entry->kind == PART_KIND_WIRE)
    return false;

  int hole_indices[PART_TERMINAL_CAPACITY];
  int hole_count = compute_placement_hole_indices(board, hole_indices);

  return touches_rail(hole_indices, hole_count);
}

static void
place_part(Game *game) {
  Breadboard *board = game->board;
  int part_count = board->part_count;

  if (is_placement_on_rail(board)) {
    snprintf(board->message, sizeof(board->message), "rails: wire only");
    return;
  }

  handle_placement_key(board, KEY_ENTER);

  if (board->part_count > part_count)
    deal_next_part(game);
}

static void
redo_first_pin(Breadboard *board) {
  const PlacementEntry *entry = board->placing_entry;

  cancel_placement(board);
  board->placing_entry = entry;
}

static void
toggle_pause(Game *game) {
  Breadboard *board = game->board;

  if (game->paused) {
    board->placing_entry = game->paused_entry;
    game->paused = false;
    return;
  }

  game->paused_entry = board->placing_entry;
  board->placing_entry = NULL;
  game->paused = true;
}

static void
handle_game_key(Game *game, int key) {
  Breadboard *board = game->board;
  Part *part = find_part_at(board, find_cursor_hole_index(board));

  board->message[0] = 0;
  key = decode_key(key);

  if (move_cursor_by_key(board, key))
    return;

  if (key == 'p') {
    toggle_pause(game);
    return;
  }

  if (game->paused && key == 'i') {
    if (part)
      show_part_info(part->kind);
    return;
  }

  if (game->paused && key != 'q')
    return;

  switch (key) {
  case KEY_ENTER:
    place_part(game);
    break;
  case ' ':
    if (part)
      operate_part(board, part);
    break;
  case '+':
  case '=':
    if (part)
      adjust_part(board, part, 1);
    break;
  case '-':
    if (part)
      adjust_part(board, part, -1);
    break;
  case KEY_ESCAPE:
    redo_first_pin(board);
    break;
  case 'q':
    if (widget_run_confirm("Quit game?"))
      game->quit = true;
    break;
  default:
    break;
  }
}

static void
update_game(Game *game) {
  Breadboard *board = game->board;

  step_simulation(board);

  if (has_burned_part(board)) {
    game->over_reason = GAME_OVER_BURNED;
    return;
  }

  if (board->short_circuit) {
    game->over_reason = GAME_OVER_SHORT;
    return;
  }

  if (game->clear_frames > 0) {
    game->clear_frames--;

    if (game->clear_frames == 0) {
      game->score += remove_marked_parts(board);
      game->room_check_pending = true;
    }

    return;
  }

  if (has_active_output(board)) {
    mark_current_path_parts(board);
    game->clear_frames = CLEAR_FRAMES;
    return;
  }

  if (board->part_count - count_parts_of_kind(board, PART_KIND_BATTERY) >
      PART_LIMIT) {
    game->over_reason = GAME_OVER_TOO_MANY_PARTS;
    return;
  }

  if (game->room_check_pending) {
    game->room_check_pending = false;

    if (!can_place_anywhere(board, board->placing_entry->kind))
      game->over_reason = GAME_OVER_NO_ROOM;
  }
}

static void
draw_clear_markers(const Breadboard *board) {
  if (board->frame_count % 2 != 0)
    return;

  for (int i = 0; i < board->part_count; i++) {
    const Part *part = &board->parts[i];

    if (!is_part_marked(i))
      continue;

    for (int j = 0; j < part->terminal_count; j++)
      canvas_rect(
        compute_hole_x(part->terminal_hole_indices[j]) - 3,
        compute_hole_y(part->terminal_hole_indices[j]) - 3,
        7,
        7,
        CLEAR_MARKER_COLOR
      );
  }
}

static void
format_game_header_right(const Game *game, char *text, size_t text_size) {
  if (game->board->message[0]) {
    snprintf(text, text_size, "%s", game->board->message);
    return;
  }

  if (game->clear_frames > 0) {
    snprintf(text, text_size, "CLEAR!");
    return;
  }

  snprintf(
    text,
    text_size,
    "SCORE %d  %d/%d",
    game->score,
    game->board->part_count -
      count_parts_of_kind(game->board, PART_KIND_BATTERY),
    PART_LIMIT
  );
}

static void
format_pause_header_right(Breadboard *board, char *text, size_t text_size) {
  const Part *part = find_part_at(board, find_cursor_hole_index(board));

  if (part == NULL) {
    snprintf(text, text_size, "PAUSE");
    return;
  }

  format_part_reading(part, text, text_size);
}

static void
draw_rail_markers(const Breadboard *board) {
  int hole_indices[PART_TERMINAL_CAPACITY];
  int hole_count = compute_placement_hole_indices(board, hole_indices);

  for (int i = 0; i < hole_count; i++) {
    if (!is_rail_hole(hole_indices[i]))
      continue;

    canvas_rect(
      compute_hole_x(hole_indices[i]) - 3,
      compute_hole_y(hole_indices[i]) - 3,
      7,
      7,
      RAIL_BLOCKED_COLOR
    );
  }
}

static void
draw_game(Game *game) {
  Breadboard *board = game->board;
  char title[TEXT_SIZE];
  char right_text[TEXT_SIZE];

  canvas_fill(THEME_BACKGROUND_COLOR);
  draw_board();
  draw_parts(board);

  if (game->clear_frames > 0) {
    draw_clear_markers(board);
  } else if (!game->paused) {
    draw_placement_preview(board);

    if (is_placement_on_rail(board))
      draw_rail_markers(board);
  }

  draw_cursor(board);

  if (game->paused) {
    format_header_title(board, title, sizeof title);
    format_pause_header_right(board, right_text, sizeof right_text);
  } else {
    format_placement_title(board, title, sizeof title);
    format_game_header_right(game, right_text, sizeof right_text);
  }

  widget_draw_header(title, right_text);
}

static const char *
find_game_over_text(GameOverReason reason) {
  switch (reason) {
  case GAME_OVER_BURNED:
    return "A part burned out";
  case GAME_OVER_SHORT:
    return "Short circuit";
  case GAME_OVER_TOO_MANY_PARTS:
    return "Too many parts";
  default:
    return "No room left";
  }
}

// The browser build keeps no hiscore, so it shows none.
#if !defined(PIN_BOARD_WASM)
static void
record_hiscore(const Game *game, char *text, size_t text_size) {
  if (game->score <= game->hiscore) {
    snprintf(text, text_size, "HI-SCORE %d", game->hiscore);
    return;
  }

  if (!save_hiscore(game->score)) {
    snprintf(text, text_size, "HI-SCORE save failed");
    return;
  }

  snprintf(text, text_size, "NEW HI-SCORE!");
}
#endif

static void
show_game_over(Game *game) {
  char score_line[TEXT_SIZE];

  snprintf(score_line, sizeof score_line, "SCORE %d", game->score);

#if !defined(PIN_BOARD_WASM)
  char hiscore_line[TEXT_SIZE];

  record_hiscore(game, hiscore_line, sizeof hiscore_line);
#endif

  const char *lines[GAME_OVER_LINE_COUNT] = {
    "GAME OVER",
    find_game_over_text(game->over_reason),
    score_line,
#if !defined(PIN_BOARD_WASM)
    hiscore_line,
#endif
    "any key  title",
  };

  while (canvas_begin_band()) {
    draw_game(game);
    widget_draw_panel(
      GAME_OVER_PANEL_X,
      GAME_OVER_PANEL_Y,
      GAME_OVER_PANEL_WIDTH,
      GAME_OVER_LINE_COUNT * GAME_OVER_LINE_HEIGHT + 8
    );

    for (int i = 0; i < GAME_OVER_LINE_COUNT; i++)
      canvas_text(
        GAME_OVER_PANEL_X + 8,
        GAME_OVER_PANEL_Y + 4 + i * GAME_OVER_LINE_HEIGHT,
        lines[i],
        i == 0 ? THEME_EMPHASIS_COLOR : THEME_TEXT_COLOR
      );

    canvas_push_band();
  }

  keyboard_wait_key();
}

void
run_game(Breadboard *board) {
  Game game = {0};

  game.board = board;
  game.hiscore = load_hiscore();
  reset_board(board);
  place_battery(board);
  deal_next_part(&game);

  while (!game.quit && game.over_reason == GAME_OVER_NONE) {
    int key = keyboard_read_key();

    if (key != KEY_NONE && game.clear_frames == 0)
      handle_game_key(&game, key);

    if (game.quit)
      break;

    if (!game.paused)
      update_game(&game);

    while (canvas_begin_band()) {
      draw_game(&game);
      canvas_push_band();
    }

    board->frame_count++;
    vTaskDelay(pdMS_TO_TICKS(GAME_FRAME_INTERVAL_MS));
  }

  if (game.over_reason != GAME_OVER_NONE)
    show_game_over(&game);

  reset_board(board);
}
