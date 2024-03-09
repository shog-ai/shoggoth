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

typedef struct {
  node_ctx_t *ctx;
} tunnel_monitor_thread_args_t;

void *tunnel_monitor(void *thread_arg);

result_t kill_tunnel_server(node_ctx_t *ctx);

result_t kill_tunnel_client(node_ctx_t *ctx);

result_t launch_tunnel_client(node_ctx_t *ctx, u64 custom_port);

void launch_tunnel_server(node_ctx_t *ctx);

#endif
