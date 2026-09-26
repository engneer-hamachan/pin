#include "breadboard.h"

#include "machine.h"
#include "picoruby.h"
#include "task.h"

#include <mrc_diagnostic.h>
#include <mruby/string.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RUBY_HEAP_SIZE (128 * 1024)
#define RUBY_HEAP_ALIGNMENT 8
#define STEP_BUDGET_US 20000
#define PROGRAM_TASK_NAME "pino"
#define MESSAGE_TEXT_SIZE 96

extern mrb_state *global_mrb;

typedef struct {
  const uint8_t *source;
  size_t source_byte_count;
  mrc_ccontext *compile_context;
  mrb_value task;
} ProgramStart;

static uint8_t *heap_memory = NULL;
static mrb_state *ruby_state = NULL;
static char *program_source = NULL;
static mrb_value program_task;
static PinoProgramState program_state = PINO_PROGRAM_STOPPED;
static bool program_powered = false;
static char console_lines[PINO_CONSOLE_LINE_COUNT][PINO_CONSOLE_LINE_SIZE];
static int console_line_count = 0;
static int console_column = 0;
static bool console_line_open = false;

static void
start_console_line(void) {
  if (console_line_count == PINO_CONSOLE_LINE_COUNT) {
    memmove(
      console_lines[0],
      console_lines[1],
      (PINO_CONSOLE_LINE_COUNT - 1) * PINO_CONSOLE_LINE_SIZE
    );
    console_line_count--;
  }

  console_lines[console_line_count][0] = 0;
  console_line_count++;
  console_column = 0;
  console_line_open = true;
}

void
write_pino_console(const char *text, int byte_count) {
  for (int i = 0; i < byte_count; i++) {
    if (text[i] == '\n') {
      console_line_open = false;
      continue;
    }

    if (text[i] == '\r')
      continue;

    if (!console_line_open || console_column == PINO_CONSOLE_LINE_SIZE - 1)
      start_console_line();

    char *line = console_lines[console_line_count - 1];

    line[console_column++] = text[i];
    line[console_column] = 0;
  }
}

static void
clear_console(void) {
  console_line_count = 0;
  console_column = 0;
  console_line_open = false;
}

static void
write_console_text(const char *text) {
  write_pino_console(text, (int)strlen(text));
}

int
count_pino_console_lines(void) {
  return console_line_count;
}

const char *
read_pino_console_line(int line_index) {
  return console_lines[line_index];
}

static bool
open_ruby_state(void) {
  if (ruby_state != NULL)
    return true;

  heap_memory = malloc(RUBY_HEAP_SIZE + RUBY_HEAP_ALIGNMENT);

  if (heap_memory == NULL)
    return false;

  uintptr_t address = (uintptr_t)heap_memory;
  uintptr_t aligned_address = (address + RUBY_HEAP_ALIGNMENT - 1) &
                              ~(uintptr_t)(RUBY_HEAP_ALIGNMENT - 1);

  ruby_state =
    mrb_open_with_custom_alloc((void *)aligned_address, RUBY_HEAP_SIZE);

  if (MRB_OPEN_FAILURE(ruby_state)) {
    if (ruby_state != NULL)
      mrb_close(ruby_state);

    ruby_state = NULL;
    free(heap_memory);
    heap_memory = NULL;
    return false;
  }

  global_mrb = ruby_state;
  return true;
}

static void
close_ruby_state(void) {
  if (ruby_state == NULL)
    return;

  mrb_hal_task_final(ruby_state);
  mrb_close(ruby_state);
  ruby_state = NULL;
  global_mrb = NULL;
  free(heap_memory);
  heap_memory = NULL;
  free(program_source);
  program_source = NULL;
}

static char *
read_program_file(size_t *byte_count) {
  if (!mount_sd())
    return NULL;

  FILE *file = fopen(PINO_PROGRAM_PATH, "rb");
  char *source = NULL;

  if (file != NULL) {
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);

    fseek(file, 0, SEEK_SET);

    if (file_size >= 0)
      source = malloc((size_t)file_size + 1);

    if (source != NULL) {
      *byte_count = fread(source, 1, (size_t)file_size, file);
      source[*byte_count] = 0;
    }

    fclose(file);
  }

  unmount_sd();
  return source;
}

static void
write_compile_errors(const mrc_ccontext *compile_context) {
  const mrc_diagnostic_list *diagnostic = compile_context->diagnostic_list;

  while (diagnostic != NULL) {
    if (diagnostic->code == MRC_PARSER_ERROR ||
        diagnostic->code == MRC_GENERATOR_ERROR) {
      char text[MESSAGE_TEXT_SIZE];

      snprintf(
        text,
        sizeof text,
        "line %u: %s\n",
        (unsigned int)diagnostic->line,
        diagnostic->message
      );
      write_console_text(text);
    }

    diagnostic = diagnostic->next;
  }
}

static mrb_value
compile_program(mrb_state *mrb, void *userdata) {
  ProgramStart *start = (ProgramStart *)userdata;
  const uint8_t *source = start->source;
  mrc_irep *irep = mrc_load_string_cxt(
    start->compile_context,
    &source,
    start->source_byte_count
  );

  if (irep == NULL) {
    write_compile_errors(start->compile_context);
    return mrb_nil_value();
  }

  start->task = mrc_create_task(
    start->compile_context,
    irep,
    mrb_str_new_cstr(mrb, PROGRAM_TASK_NAME),
    mrb_nil_value(),
    mrb_obj_value(mrb->top_self)
  );
  return start->task;
}

static void
write_exception(mrb_value exception) {
  mrb_value text = mrb_inspect(ruby_state, exception);

  if (ruby_state->exc != NULL) {
    ruby_state->exc = NULL;
    write_console_text("exception\n");
    return;
  }

  write_pino_console(RSTRING_PTR(text), (int)RSTRING_LEN(text));
  write_console_text("\n");
}

static void
finish_program(PinoProgramState state) {
  program_state = state;
  close_ruby_state();
}

static void
start_program(void) {
  stop_pino_program();
  clear_console();

  size_t source_byte_count = 0;

  program_source = read_program_file(&source_byte_count);

  if (program_source == NULL) {
    write_console_text("no " PINO_PROGRAM_FILENAME "\n");
    program_state = PINO_PROGRAM_FAILED;
    return;
  }

  char *source = program_source;

  program_source = NULL;

  if (!open_ruby_state()) {
    free(source);
    write_console_text("no memory for ruby\n");
    program_state = PINO_PROGRAM_FAILED;
    return;
  }

  program_source = source;

  ProgramStart start = {
    .source = (const uint8_t *)source,
    .source_byte_count = source_byte_count,
    .compile_context = mrc_ccontext_new(ruby_state),
    .task = mrb_nil_value(),
  };

  if (start.compile_context == NULL) {
    write_console_text("no memory for compiler\n");
    finish_program(PINO_PROGRAM_FAILED);
    return;
  }

  mrb_bool error = FALSE;
  mrb_value result =
    mrb_protect_error(ruby_state, compile_program, &start, &error);

  mrc_ccontext_free(start.compile_context);

  if (error) {
    write_exception(result);
    finish_program(PINO_PROGRAM_FAILED);
    return;
  }

  if (mrb_nil_p(start.task)) {
    finish_program(PINO_PROGRAM_FAILED);
    return;
  }

  program_task = start.task;
  mrb_gc_register(ruby_state, program_task);
  program_state = PINO_PROGRAM_RUNNING;
}

void
stop_pino_program(void) {
  if (program_state == PINO_PROGRAM_RUNNING)
    program_state = PINO_PROGRAM_STOPPED;

  close_ruby_state();
  reset_pino_gpios();
}

static mrb_value
run_task_once(mrb_state *mrb, void *userdata) {
  (void)userdata;
  return mrb_task_run_once(mrb);
}

static bool
is_program_task_dormant(void) {
  mrb_value status = mrb_task_status(ruby_state, program_task);

  return mrb_symbol_p(status) &&
         mrb_symbol(status) == mrb_intern_lit(ruby_state, "DORMANT");
}

static void
finish_dormant_program(void) {
  mrb_value result = mrb_task_value(ruby_state, program_task);

  if (mrb_exception_p(result)) {
    write_exception(result);
    finish_program(PINO_PROGRAM_FAILED);
    return;
  }

  finish_program(PINO_PROGRAM_FINISHED);
}

void
restart_pino_program(void) {
  if (is_pino_powered())
    start_program();
}

void
step_pino_program(void) {
  bool powered = is_pino_powered();

  if (powered && !program_powered)
    start_program();

  program_powered = powered;

  if (program_state != PINO_PROGRAM_RUNNING)
    return;

  uint64_t start_us = Machine_uptime_us();

  while (Machine_uptime_us() - start_us < STEP_BUDGET_US) {
    if (!is_pino_powered()) {
      stop_pino_program();
      write_console_text("power off\n");
      return;
    }

    advance_pino_ticks();

    mrb_bool error = FALSE;
    mrb_value result =
      mrb_protect_error(ruby_state, run_task_once, NULL, &error);

    if (error) {
      write_exception(result);
      finish_program(PINO_PROGRAM_FAILED);
      return;
    }

    if (is_program_task_dormant()) {
      finish_dormant_program();
      return;
    }

    if (mrb_nil_p(result))
      return;
  }
}

PinoProgramState
read_pino_program_state(void) {
  return program_state;
}
