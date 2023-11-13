/**** dht.c ****
 *
 *  Copyright (c) 2023 Shoggoth Systems - https://shoggoth.systems
 *
 * Part of the ShogDB database, under the MIT License.
 * See LICENCE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#include "dht.h"

db_ctx_t *dht_ctx = NULL;

result_t setup_dht(db_ctx_t *ctx) {
  dht_ctx = ctx;

  cJSON *value_json = cJSON_CreateArray();

  db_add_json_value(ctx, "dht", value_json);

  cJSON_Delete(value_json);

  return OK(NULL);
}
