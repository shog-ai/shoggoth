/**** pins.h ****
 *
 *  Copyright (c) 2023 ShogAI - https://shog.ai
 *
 * Part of the ShogDB database, under the MIT License.
 * See LICENSE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#ifndef SHOGDB_PINS_H
#define SHOGDB_PINS_H

#include <netlibc/error.h>
#include <netlibc/fs.h>

#include "../../include/sonic.h"
#include "db.h"

result_t setup_pins(db_ctx_t *ctx);

void add_pins_routes(sonic_server_t *server);

#endif
