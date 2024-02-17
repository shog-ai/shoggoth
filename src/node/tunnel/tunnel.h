/**** tunnel.h ****
 *
 *  Copyright (c) 2023 ShogAI - https://shog.ai
 *
 * Part of the Shoggoth project, under the MIT License.
 * See LICENSE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#ifndef NODE_TUNNEL_H
#define NODE_TUNNEL_H

#include "../dht/dht.h"

result_t kill_tunnel_server(node_ctx_t *ctx);

void launch_tunnel_client(node_ctx_t *ctx);

void launch_tunnel_server(node_ctx_t *ctx);

#endif
