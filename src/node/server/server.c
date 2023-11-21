/**** server.c ****
 *
 *  Copyright (c) 2023 Shoggoth Systems
 *
 * Part of the Shoggoth project, under the MIT License.
 * See LICENCE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#include "../../json/json.h"
#include "../og/og.h"
#include "api.h"
#include "server_explorer.h"

/****
 * starts a http server exposing the Shoggoth Node API and Shoggoth Explorer
 *
 ****/
void *start_node_server(void *thread_args) {
  server_thread_args_t *args = (server_thread_args_t *)thread_args;

  sonic_server_t *server = sonic_new_server(args->ctx->config->network.host,
                                            args->ctx->config->network.port);

  sonic_add_server_rate_limiter_middleware(
      server, args->ctx->config->api.rate_limiter_requests,
      args->ctx->config->api.rate_limiter_duration,
      "you are being rate limited.");

  if (!args->ctx->config->api.enable) {
    LOG(WARN, "Node API disabled");

    return NULL;
  }

  add_api_routes(args->ctx, server);

  if (args->ctx->config->explorer.enable) {
    add_explorer_routes(args->ctx, server);
  } else {
    LOG(WARN, "Node Explorer disabled");
  }

  args->ctx->node_http_server = server;
  args->ctx->node_server_started = true;

  og_init_stb(args->ctx);
  int failed = sonic_start_server(server);
  og_deinit_stb();
  if (failed) {
    LOG(ERROR, "start node server failed");
    shoggoth_node_exit(1);
  }

  LOG(INFO, "node server exited");

  return NULL;
}