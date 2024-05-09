/**** json.h ****
 *
 *  Copyright (c) 2023 ShogAI - https://shog.ai
 *
 * Part of the ShogDB database, under the MIT License.
 * See LICENSE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#ifndef SHOGDB_JSON_H
#define SHOGDB_JSON_H

#include <netlibc/error.h>
#include <netlibc/fs.h>

#include "../../include/sonic.h"
#include "db.h"

void add_json_routes(db_ctx_t *ctx, sonic_server_t *server);

void *str_to_json(char *str);
char *json_to_str(void *json);
void free_json(void *json);

#endif
