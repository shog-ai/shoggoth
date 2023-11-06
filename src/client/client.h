/**** client.h ****
 *
 *  Copyright (c) 2023 Shoggoth Systems
 *
 * Part of the Shoggoth project, under the MIT License.
 * See LICENCE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#ifndef SHOG_CLIENT_H
#define SHOG_CLIENT_H

#include <netlibc.h>

#include "../args/args.h"

typedef struct {
  char *public_key;
  char *shoggoth_id;
} client_manifest_t;

typedef struct {
  char **nodes;
  u64 nodes_count;
} known_nodes_t;

typedef struct {
} client_config_t;

typedef struct CLIENT_CTX {
  client_config_t config;
  client_manifest_t *manifest;
  known_nodes_t* known_nodes;
  char *runtime_path;
} client_ctx_t;

result_t client_delegate_node(client_ctx_t *ctx, char node_host[]);

result_t handle_client_session(args_t *args);

#endif