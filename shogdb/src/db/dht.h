/**** dht.h ****
 *
 *  Copyright (c) 2023 ShogAI - https://shog.ai
 *
 * Part of the ShogDB database, under the MIT License.
 * See LICENSE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#ifndef SHOGDB_DHT_H
#define SHOGDB_DHT_H

#include <netlibc/error.h>
#include <netlibc/fs.h>

#include "../../include/sonic.h"
#include "db.h"

result_t setup_dht(db_ctx_t *ctx);

void add_dht_routes(sonic_server_t *server);

#endif
