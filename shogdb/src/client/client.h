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

typedef struct {
  char *address;
} shogdb_ctx_t;

typedef enum {
  VALUE_NULL,
  VALUE_ERR,

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

  char *value_err;

  // integer types
  u64 value_uint;
  s64 value_int;

  f64 value_float;

  char *value_str;
  bool value_bool;
  void *value_json;
} db_value_t;

result_t shogdb_set_int(shogdb_ctx_t *ctx, char *key, s64 value);
result_t shogdb_set_uint(shogdb_ctx_t *ctx, char *key, u64 value);
result_t shogdb_set_float(shogdb_ctx_t *ctx, char *key, f64 value);
result_t shogdb_set_str(shogdb_ctx_t *ctx, char *key, char *value);
result_t shogdb_set_bool(shogdb_ctx_t *ctx, char *key, bool value);
result_t shogdb_set_json(shogdb_ctx_t *ctx, char *key, char *value);
result_t shogdb_print(shogdb_ctx_t *ctx);

result_t shogdb_json_append(shogdb_ctx_t *ctx, char *key, char *filter,
                            char *value);

result_t shogdb_json_get(shogdb_ctx_t *ctx, char *key, char *filter);

result_t shogdb_get(shogdb_ctx_t *ctx, char *key);

result_t shogdb_delete(shogdb_ctx_t *ctx, char *key);

shogdb_ctx_t *new_shogdb(char *address);
void free_shogdb(shogdb_ctx_t *ctx);

db_value_t *new_db_value(value_type_t value_type);

value_type_t str_to_value_type(char *str);

char *value_type_to_str(value_type_t value_type);

void free_db_value(db_value_t *value);

result_t shogdb_parse_message(char *msg);

#endif
