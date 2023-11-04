/**** camel.h ****
 *
 *  Copyright (c) 2023 Shoggoth Systems - https://shoggoth.systems
 *
 * Part of the Camel testing framework, under the MIT License.
 * See LICENCE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#ifndef CAMEL_H
#define CAMEL_H

#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// DEFINITIONS

#define SETUP_VALUE __setup_result
#define VALUE_T void *
#define ERROR_COLOR camel_rgb(227, 61, 45)
#define SUCCESS_COLOR camel_rgb(61, 242, 41)

// MACROS

#define CAMEL_BEGIN(runner_type)                                               \
  camel_ctx_t *__global_ctx = __init_camel_ctx();                              \
  __init_runner(__global_ctx, argc, argv, __FILE__, runner_type);

#define ADD_SUITE(suite_func) __add_suite(__global_ctx, suite_func, #suite_func)

#define ADD_TEST(test_func) __add_test(suite, test_func, #test_func, 1)

#define ADD_REPEAT_TEST(test_func, repeat_count)                               \
  __add_test(suite, test_func, #test_func, repeat_count)

#define ADD_SETUP(test_case, setup_func)                                       \
  __add_setup(test_case, setup_func, #setup_func)

#define ADD_TEARDOWN(test_case, teardown_func)                                 \
  __add_teardown(test_case, teardown_func, #teardown_func)

#define CAMEL_END()                                                            \
  do {                                                                         \
    return __run_all(__global_ctx);                                            \
  } while (0)

#define INIT_SETUP(value_type)                                                 \
  value_type *SETUP_VALUE = malloc(sizeof(value_type));

#define FREE_SETUP()                                                           \
  do {                                                                         \
    free(SETUP_VALUE);                                                         \
  } while (0)

#define USE_SETUP(value_type)                                                  \
  if (!test->has_setup) {                                                      \
    TEST_FAIL("There is no setup");                                            \
  }                                                                            \
  value_type *__setup_result = (value_type *)test->setup->setup_result;

#define SETUP_RETURN()                                                         \
  do {                                                                         \
    return SETUP_VALUE;                                                        \
  } while (0)

#define TEST_PASS(message)                                                     \
  do {                                                                         \
    return;                                                                    \
  } while (0)

#define TEST_FAIL(message)                                                     \
  do {                                                                         \
    camel_tui_set_color_rgb(ERROR_COLOR);                                      \
    printf("FORCE_TEST_FAIL: %s\n", message);                                  \
    FAILED();                                                                  \
    return;                                                                    \
  } while (0);

#define FAILED()                                                               \
  do {                                                                         \
    test->test_failed = true;                                                  \
    return;                                                                    \
  } while (0)

// ASSERTS

#define ASSERT(condition, message)                                             \
  do {                                                                         \
    test->total_assertions++;                                                  \
    if (!(condition)) {                                                        \
      camel_tui_set_color_rgb(ERROR_COLOR);                                    \
      printf("ASSERT FAILED: %s\n", message);                                  \
      FAILED();                                                                \
    }                                                                          \
  } while (0)

#define ASSERT_FALSE(condition, message)                                       \
  do {                                                                         \
    test->total_assertions++;                                                  \
    if ((condition)) {                                                         \
      camel_tui_set_color_rgb(ERROR_COLOR);                                    \
      printf("ASSERT_FALSE FAILED: %s\n", message);                            \
      FAILED();                                                                \
    }                                                                          \
  } while (0)

#define ASSERT_EQ(left, right, message)                                        \
  do {                                                                         \
    test->total_assertions++;                                                  \
    if (!(left == right)) {                                                    \
      camel_tui_set_color_rgb(ERROR_COLOR);                                    \
      printf("ASSERT_EQ FAILED: %s\n", "values are not equal");                \
      printf("      -> EXPECTED (%s) = %d\n", #right, right);                  \
      printf("      -> ACTUAL (%s) = %d\n", #left, left);                      \
      FAILED();                                                                \
    }                                                                          \
  } while (0)

#define ASSERT_NEQ(left, right, message)                                       \
  do {                                                                         \
    test->total_assertions++;                                                  \
    if ((left == right)) {                                                     \
      camel_tui_set_color_rgb(ERROR_COLOR);                                    \
      printf("ASSERT_NEQ FAILED: %s\n", "values are equal");                   \
      printf("      -> EXPECTED (%s) = %d\n", #right, right);                  \
      printf("      -> ACTUAL (%s) = %d\n", #left, left);                      \
      FAILED();                                                                \
    }                                                                          \
  } while (0)

#define ASSERT_NULL(value, message)                                            \
  do {                                                                         \
    test->total_assertions++;                                                  \
    if (!(value == NULL)) {                                                    \
      camel_tui_set_color_rgb(ERROR_COLOR);                                    \
      printf("ASSERT_NULL FAILED: %s\n", "value is not NULL");                 \
      FAILED();                                                                \
    }                                                                          \
  } while (0)

#define ASSERT_NOT_NULL(value, message)                                        \
  do {                                                                         \
    test->total_assertions++;                                                  \
    if ((value == NULL)) {                                                     \
      camel_tui_set_color_rgb(ERROR_COLOR);                                    \
      printf("ASSERT_NOT_NULL FAILED: %s\n", "value is NULL");                 \
      FAILED();                                                                \
    }                                                                          \
  } while (0)

#define ASSERT_EQ_STR(left, right, message)                                    \
  do {                                                                         \
    test->total_assertions++;                                                  \
    char *right_value = "(null)";                                              \
    char *left_value = "(null)";                                               \
    if (right != NULL) {                                                       \
      right_value = (char *)right;                                             \
    }                                                                          \
    if (left != NULL) {                                                        \
      left_value = (char *)left;                                               \
    }                                                                          \
    if (!(strcmp(left_value, right_value) == 0)) {                             \
      camel_tui_set_color_rgb(ERROR_COLOR);                                    \
      printf("ASSERT_EQ_STR FAILED: %s\n", "strings are not equal");           \
      printf("      -> EXPECTED (%s) = \"%s\"\n", #right, right_value);        \
      printf("      -> ACTUAL (%s) = \"%s\"\n", #left, left_value);            \
      FAILED();                                                                \
    }                                                                          \
  } while (0)

#define ASSERT_EQ_BOOL(left, right, message)                                   \
  do {                                                                         \
    test->total_assertions++;                                                  \
    if (!(left == right)) {                                                    \
      camel_tui_set_color_rgb(ERROR_COLOR);                                    \
      printf("ASSERT_EQ_BOOL FAILED: %s\n", "boolean values are not equal");   \
      printf("      -> EXPECTED (%s) = %s\n", #right,                          \
             left == true ? "true" : "false");                                 \
      printf("      -> ACTUAL (%s) = %s\n", #left,                             \
             right == true ? "true" : "false");                                \
      FAILED();                                                                \
    }                                                                          \
  } while (0)

#define RUN_EXECUTABLE(output_path, executable_path, ...)                      \
  __run_executable(output_path, executable_path, __VA_ARGS__);

#define KILL_PROCESS(process, signal) __kill_process(process, signal);

#define AWAIT_PROCESS(process) __await_process(process);

#define READ_FILE(path) __read_file(path);

#define FILES_EQUAL(first_file, second_file)                                   \
  __files_equal(first_file, second_file);

// TYPES

struct TEST_CASE;
struct SUITE;

typedef struct CAMEL_RGB_TYPE {
  uint8_t r;
  uint8_t g;
  uint8_t b;
} camel_rgb_t;

typedef void (*suite_func_t)(struct SUITE *test);

typedef void (*test_func_t)(struct TEST_CASE *test);

typedef VALUE_T (*setup_func_t)();

typedef void (*teardown_func_t)(struct TEST_CASE *test);

typedef struct {
  char *setup_name;
  setup_func_t setup_func;
  void *setup_result;
} setup_t;

typedef struct {
  char *teardown_name;
  teardown_func_t teardown_func;
} teardown_t;

typedef struct TEST_CASE {
  char *test_name;
  test_func_t test_func;

  bool has_setup;
  setup_t *setup;
  bool has_teardown;
  teardown_t *teardown;
  uint32_t total_assertions;

  bool test_failed;

  int repeat_count;
} test_t;

typedef struct SUITE {
  test_t **test_cases;
  uint32_t cases_count;
  char *suite_name;

  uint32_t passed;
  uint32_t failed;
  uint32_t total_assertions;
} suite_t;

typedef enum {
  UNIT,
  INTEGRATION,
  FUNCTIONAL,
  FUZZ,
} runner_type_t;

typedef struct {
  suite_t **test_suites;
  uint32_t suites_count;

  uint32_t passed;
  uint32_t failed;
  uint32_t total_assertions;
  uint32_t total_tests_count;

  char *runner_name;

  runner_type_t runner_type;
} runner_t;

typedef struct {
  runner_t *runner;
  bool fail_fast;
  bool parallel;
  bool return_error_code;
} camel_ctx_t;

typedef struct {
  suite_t *suite;
  test_t *test_case;
  camel_ctx_t *ctx;
} test_args_t;

typedef struct {
  pid_t pid;
  char *binary_path;
} process_t;

// FUNCTIONS

bool __files_equal(char *first_file, char *second_file);

process_t *__run_executable(char *output_path, char *executable_path, ...);

void __kill_process(process_t *process, int signal);

int __await_process(process_t *process);

char *__read_file(char *path);

camel_ctx_t *__init_camel_ctx();

void __init_runner(camel_ctx_t *ctx, int argc, char **argv, char *file_name,
                   runner_type_t runner_type);

setup_t *__add_setup(test_t *test_case, setup_func_t setup_func,
                     char *setup_name);

teardown_t *__add_teardown(test_t *test_case, teardown_func_t teardown_func,
                           char *teardown_name);

void __add_suite(camel_ctx_t *ctx, suite_func_t suite_func,
                 const char *suite_name);

test_t *__add_test(suite_t *test_suite, test_func_t test_func, char *test_name,
                   int repeat_count);

int __run_all(camel_ctx_t *ctx);

void __fail_fast(camel_ctx_t *ctx);

camel_rgb_t camel_rgb(uint8_t r, uint8_t g, uint8_t b);

void camel_tui_set_color_rgb(camel_rgb_t color);

void camel_tui_reset_color();

#endif
