/**** server_explorer.h ****
 *
 *  Copyright (c) 2023 ShogAI - https://shog.ai
 *
 * Part of the Shoggoth project, under the MIT License.
 * See LICENSE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#ifndef SHOG_NODE_SERVER_EXPLORER_H
#define SHOG_NODE_SERVER_EXPLORER_H

#include "../../sonic/sonic.h"
#include "../node.h"

void add_explorer_routes(node_ctx_t *ctx, sonic_server_t *server);

#endif
