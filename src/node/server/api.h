/**** api.h ****
 *
 *  Copyright (c) 2023 Shoggoth Systems
 *
 * Part of the Shoggoth project, under the MIT License.
 * See LICENCE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#ifndef SHOG_NODE_SERVER_API_H
#define SHOG_NODE_SERVER_API_H

#include "../node.h"
#include "../../include/sonic.h"

void respond_error(sonic_server_request_t *req, char *error_message);
void respond_shog_id_invalid(sonic_server_request_t *req, char *shoggoth_id);
void add_api_routes(node_ctx_t *ctx, sonic_server_t *server);

#endif