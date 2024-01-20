/**** error.c ****
 *
 *  Copyright (c) 2023 ShogAI - https://shog.ai
 *
 * Part of netlibc, under the MIT License.
 * See LICENCE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#include "../include/netlibc/log.h"
#include "../include/netlibc/string.h"

#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>

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
