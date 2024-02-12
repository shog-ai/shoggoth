/**** api.h ****
 *
 *  Copyright (c) 2023 ShogAI - https://shog.ai
 *
 * Part of the Shoggoth project, under the MIT License.
 * See LICENSE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#ifndef SHOG_NODE_SERVER_API_H
#define SHOG_NODE_SERVER_API_H

#include "../node.h"
#include "../../include/sonic.h"

void respond_error(sonic_server_request_t *req, char *error_message);
void add_api_routes(node_ctx_t *ctx, sonic_server_t *server);

#endif
