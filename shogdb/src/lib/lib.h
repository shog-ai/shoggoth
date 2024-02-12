/**** lib.h ****
 *
 *  Copyright (c) 2023 ShogAI - https://shog.ai
 *
 * Part of the ShogDB database, under the MIT License.
 * See LICENSE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#ifndef SHOGDB_LIB_H
#define SHOGDB_LIB_H

#include <netlibc.h>
#include <netlibc/error.h>

#include <pthread.h>

typedef enum {
  VALUE_NULL,

  VALUE_UINT,
  VALUE_INT,
  VALUE_FLOAT,

  VALUE_STR,
  VALUE_BOOL,
  VALUE_JSON
} value_type_t;

typedef struct {
  pthread_mutex_t mutex;

  value_type_t value_type;

  // integer types
  u64 value_uint;
  s64 value_int;

  f64 value_float;

  char *value_str;
  bool value_bool;
  void *value_json;
} db_value_t;

db_value_t *new_db_value(value_type_t value_type);

value_type_t str_to_value_type(char *str);

char *value_type_to_str(value_type_t value_type);

void free_db_value(db_value_t *value);

result_t shogdb_parse_message(char *msg);

#endif