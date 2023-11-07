/**** netlibc.c ****
 *
 *  Copyright (c) 2023 Shoggoth Systems - https://shoggoth.systems
 *
 * Part of netlibc, under the MIT License.
 * See LICENCE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#include "../netlibc.h"

#include <assert.h>
#include <dirent.h>
#include <execinfo.h>
#include <stdarg.h>
#include <stdlib.h>

#define RED_COLOR 227, 61, 45
#define YELLOW_COLOR 227, 224, 45

void set_color_rgb(u8 r, u8 g, u8 b) { printf("\e[38;2;%d;%d;%dm", r, g, b); }

void reset_color() { printf("\e[0m"); }

void modify_symbol_string(char *input) {
  char *openParen = strchr(input, '(');
  if (openParen != NULL) {
    *openParen = ' ';
    char *closeParen = strchr(openParen, ')');
    if (closeParen != NULL) {
      *(closeParen) = '\0';
    }
  }
}

void print_backtrace() {
  void *call_stack[10];
  int num_frames = backtrace(call_stack, 10);
  char **symbols = backtrace_symbols(call_stack, num_frames);

  if (symbols == NULL) {
    fprintf(stdout, "Failed to retrieve the backtrace symbols.\n");
    return;
  }

  fprintf(stdout, "Backtrace:\n");
  for (int i = 1; i < num_frames; i++) {
    char *current_symbol = strdup(symbols[i]);
    modify_symbol_string(current_symbol);

    fprintf(stdout, "[%d] %s\n", i, symbols[i]);

    char addr2line_command[256];
    snprintf(addr2line_command, sizeof(addr2line_command),
             "addr2line -e %s -f -p -C", current_symbol);

    FILE *addr2line_output = popen(addr2line_command, "r");
    if (addr2line_output) {
      char buffer[256];
      while (fgets(buffer, sizeof(buffer), addr2line_output) != NULL) {
        fprintf(stdout, "  %s", buffer);
      }
      pclose(addr2line_output);
    }

    free(current_symbol);
  }

  free(symbols);
}

/****
 * custom panic function. used for unrecoverable errors. Prints an error
 *message and exits the process immediatelly. This is an internal funtion
 *exposed by the PANIC macro defined in utils.h
 *
 ****/
void __netlibc_panic(const char *file, int line, const char *format, ...) {
  va_list args;
  va_start(args, format);

  set_color_rgb(RED_COLOR);
  fprintf(stdout, "[PANIC] ");
  reset_color();

  fprintf(stdout, "at %s:%d: ", file, line);

  vfprintf(stdout, format, args);
  fprintf(stdout, "\n");

  print_backtrace();

  fflush(stdout);

  va_end(args);

  exit(1);
}

void __netlibc_exit(const char *file, int line, int status, const char *format,
                    ...) {
  file = (char *)file;
  line = (int)line;

  va_list args;
  va_start(args, format);

#ifndef NDEBUG
  fprintf(stdout, "EXIT with status %d at %s:%d:\n", status, file, line);
#endif

  set_color_rgb(RED_COLOR);
  fprintf(stdout, "Error: ");
  reset_color();

  vfprintf(stdout, format, args);
  fprintf(stdout, "\n");

  fflush(stdout);

  va_end(args);

  exit(status);
}

/****
 * custom log function. sometimes used for print debbuging.
 * This is an internal funtion exposed by the LOG macro defined in utils.h
 *
 ****/
void __netlibc_log(log_level_t log_level, const char *file, int line,
                   const char *format, ...) {
  va_list args;
  va_start(args, format);

  // fprintf(stdout, "LOG at %s:%d: ", file, line);
  if (log_level == INFO) {
    fprintf(stdout, "[INFO]: ");
  } else if (log_level == WARN) {
    set_color_rgb(YELLOW_COLOR);
    fprintf(stdout, "[WARN]: ");
    reset_color();
  } else if (log_level == ERROR) {
    set_color_rgb(RED_COLOR);
    fprintf(stdout, "[ERROR] at %s:%d: ", file, line);
    reset_color();
  }

  vfprintf(stdout, format, args);

  printf("\n");

  fflush(stdout);

  va_end(args);
}

result_t __netlibc_err(const char *file, int line, const char *format, ...) {
  char *error_message = NULL;

  va_list args;
  va_start(args, format);

  char buf[512];
  vsprintf(buf, format, args);

  if (error_message == NULL) {
    error_message = malloc((strlen(buf) + 1) * sizeof(char));
    strcpy(error_message, buf);
  } else {
    error_message =
        realloc(error_message,
                (strlen(error_message) + strlen(buf) + 1) * sizeof(char));
    strcat(error_message, buf);
  }

  va_end(args);

  assert(error_message != NULL);

  return (result_t){.failed = true,
                    .ok_ptr = NULL,
                    .error_message = error_message,
                    .file = strdup(file),
                    .line = line};
}

bool is_ok(result_t res) {
  if (res.failed) {
    return false;
  } else {
    return true;
  }
}

void free_result(result_t res) {
  if (!is_ok(res)) {
    free(res.error_message);
  }

  free(res.file);
}

void *__unwrap(const char *file, int line, result_t res) {
  void *res_value = NULL;

  if (!is_ok(res)) {
    char *panic_msg =
        malloc((strlen(res.error_message) + 100 + 1) * sizeof(char));
    sprintf(panic_msg, "unwrap of error value from %s:%d -> %s", res.file,
            res.line, res.error_message);

    __netlibc_panic(file, line, panic_msg);
  } else {
    res_value = res.ok_ptr;
  }

  free_result(res);

  return res_value;
}

int __unwrap_int(const char *file, int line, result_t res) {
  int res_value = 0;

  if (!is_ok(res)) {
    char *panic_msg =
        malloc((strlen(res.error_message) + 100 + 1) * sizeof(char));
    sprintf(panic_msg, "unwrap of error value from %s:%d -> %s", res.file,
            res.line, res.error_message);

    __netlibc_panic(file, line, panic_msg);
  } else {
    res_value = res.ok_int;
  }

  free_result(res);

  return res_value;
}

u64 __unwrap_u64(const char *file, int line, result_t res) {
  u64 res_value = 0;

  if (!is_ok(res)) {
    char *panic_msg =
        malloc((strlen(res.error_message) + 100 + 1) * sizeof(char));
    sprintf(panic_msg, "unwrap of error value from %s:%d -> %s", res.file,
            res.line, res.error_message);

    __netlibc_panic(file, line, panic_msg);
  } else {
    res_value = res.ok_u64;
  }

  free_result(res);

  return res_value;
}

bool __unwrap_bool(const char *file, int line, result_t res) {
  bool res_value = false;

  if (!is_ok(res)) {
    char *panic_msg =
        malloc((strlen(res.error_message) + 100 + 1) * sizeof(char));
    sprintf(panic_msg, "unwrap of error value from %s:%d -> %s", res.file,
            res.line, res.error_message);

    __netlibc_panic(file, line, panic_msg);
  } else {
    res_value = res.ok_bool;
  }

  free_result(res);

  return res_value;
}

bool is_err(result_t res) {
  if (!res.failed) {
    return false;
  } else {
    return true;
  }
}
