/**** pins.c ****
 *
 *  Copyright (c) 2023 ShogAI - https://shog.ai
 *
 * Part of the ShogDB database, under the MIT License.
 * See LICENCE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#include "pins.h"

db_ctx_t *pins_ctx = NULL;

result_t setup_pins(db_ctx_t *ctx) {
  pins_ctx = ctx;

  cJSON *value_json = cJSON_CreateArray();

  db_add_json_value(ctx, "pins", value_json);

  cJSON_Delete(value_json);

  return OK(NULL);
}
