/**** assert.c ****
 *
 *  Copyright (c) 2023 ShogAI - https://shog.ai
 *
 * Part of netlibc, under the MIT License.
 * See LICENSE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#include "../include/netlibc/assert.h"
#include "../include/netlibc/log.h"

#include <stdlib.h>

void __assert_fail(char *prefix, char *error_message, char *file, int line) {
  set_color_rgb(RED_COLOR);
  printf("[PANIC] %s FAILED ", prefix);

  fprintf(stdout, "at %s:%d: ", file, line);

  reset_color();

  if (error_message != NULL) {
    printf("%s", error_message);
  }

  printf("\n");

  fflush(stdout);

  exit(1);
}

void __netlibc_assert_true(bool condition, char *error_message, char *file,
                           int line) {
  if (!condition) {
    __assert_fail("ASSERT_TRUE", error_message, file, line);
  }
}

void __netlibc_assert_false(bool condition, char *error_message, char *file,
                            int line) {
  if (condition) {
    __assert_fail("ASSERT_FALSE", error_message, file, line);
  }
}
