/**** netlibc.h ****
 *
 *  Copyright (c) 2023 Shoggoth Systems - https://shoggoth.systems
 *
 * Part of netlibc, under the MIT License.
 * See LICENCE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#ifndef NETLIBC_H
#define NETLIBC_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef float f32;
typedef double f64;

typedef size_t usize;

#ifdef __APPLE__ // This macro is defined on macOS
#define U64_FORMAT_SPECIFIER "%lld"
#else
#define U64_FORMAT_SPECIFIER "%lu"
#endif

#define LOOP()                                                                 \
  for (;;) {                                                                   \
  }

typedef struct {
  bool failed;
  char *file;
  int line;

  // OK
  void *ok_ptr;
  int ok_int;
  u64 ok_u64;
  bool ok_bool;

  // ERR
  u64 error_code;
  char *error_message;
} result_t;

typedef enum {
  WARN,
  INFO,
  ERROR,
} log_level_t;

#define EXIT(...) __netlibc_exit(__FILE__, __LINE__, __VA_ARGS__)

#define PANIC(...) __netlibc_panic(__FILE__, __LINE__, __VA_ARGS__)

#define LOG(log_level, ...)                                                    \
  __netlibc_log(log_level, __FILE__, __LINE__, __VA_ARGS__)

#define OK(value)                                                              \
  (result_t) {                                                                 \
    .failed = false, .ok_ptr = value, .error_message = NULL,                   \
    .file = strdup(__FILE__), .line = __LINE__                                 \
  }

#define OK_INT(value)                                                          \
  (result_t) {                                                                 \
    .failed = false, .ok_int = value, .error_message = NULL,                   \
    .file = strdup(__FILE__), .line = __LINE__                                 \
  }

#define OK_U64(value)                                                          \
  (result_t) {                                                                 \
    .failed = false, .ok_u64 = value, .error_message = NULL,                   \
    .file = strdup(__FILE__), .line = __LINE__                                 \
  }

#define OK_BOOL(value)                                                         \
  (result_t) {                                                                 \
    .failed = false, .ok_bool = value, .error_message = NULL,                  \
    .file = strdup(__FILE__), .line = __LINE__                                 \
  }

#define ERR(...) __netlibc_err(__FILE__, __LINE__, __VA_ARGS__)

// INFO: unlike PROPAGATE, you can use a function call for the parameter of
// UNWRAP
#define UNWRAP(res) __unwrap(__FILE__, __LINE__, res)
#define UNWRAP_INT(res) __unwrap_int(__FILE__, __LINE__, res)
#define UNWRAP_U64(res) __unwrap_u64(__FILE__, __LINE__, res)
#define UNWRAP_BOOL(res) __unwrap_bool(__FILE__, __LINE__, res)

#define VALUE(res) res.ok_ptr;

#define VALUE_U64(res) res.ok_u64;

// WARN: DO NOT use a function call as the parameter for PROPAGATE e.g
// PROPAGATE(my_function());
#define PROPAGATE(res)                                                         \
  res.ok_ptr;                                                                  \
  do {                                                                         \
    if (!is_ok(res)) {                                                         \
      return res;                                                              \
    }                                                                          \
    free_result(res);                                                          \
  } while (0)

#define PROPAGATE_INT(res)                                                     \
  res.ok_int;                                                                  \
  do {                                                                         \
    if (!is_ok(res)) {                                                         \
      return res;                                                              \
    }                                                                          \
    free_result(res);                                                          \
  } while (0)

#define PROPAGATE_U64(res)                                                     \
  res.ok_u64;                                                                  \
  do {                                                                         \
    if (!is_ok(res)) {                                                         \
      return res;                                                              \
    }                                                                          \
    free_result(res);                                                          \
  } while (0)

#define PROPAGATE_BOOL(res)                                                    \
  res.ok_bool;                                                                 \
  do {                                                                         \
    if (!is_ok(res)) {                                                         \
      return res;                                                              \
    }                                                                          \
    free_result(res);                                                          \
  } while (0)

result_t __netlibc_err(const char *file, int line, const char *format, ...);

bool is_ok(result_t res);

bool is_err(result_t res);

void *__unwrap(const char *file, int line, result_t res);
int __unwrap_int(const char *file, int line, result_t res);
u64 __unwrap_u64(const char *file, int line, result_t res);
bool __unwrap_bool(const char *file, int line, result_t res);

void free_result(result_t res);

void __netlibc_log(log_level_t log_level, const char *file, int line,
                   const char *format, ...);

void __netlibc_panic(const char *file, int line, const char *format, ...);

void __netlibc_exit(const char *file, int line, int status, const char *format,
                    ...);

#endif