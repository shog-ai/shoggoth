/**** dht.h ****
 *
 *  Copyright (c) 2023 ShogAI - https://shog.ai
 *
 * Part of the ShogDB database, under the MIT License.
 * See LICENCE file for license information.
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

// void get_dht_route(sonic_server_request_t *req);

#endif
