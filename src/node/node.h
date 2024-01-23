/**** node.h ****
 *
 *  Copyright (c) 2023 Shoggoth Systems
 *
 * Part of the Shoggoth project, under the MIT License.
 * See LICENCE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#ifndef SHOG_NODE_H
#define SHOG_NODE_H

#include <netlibc.h>
#include <netlibc/error.h>
#include <netlibc/fs.h>
#include <netlibc/log.h>
#include <netlibc/string.h>

#include "../include/sonic.h"

#include "../args/args.h"
#include <pthread.h>
#include <unistd.h>

typedef struct {
  char *public_host;
  char *public_key;
  char *node_id;
  char *version;
  bool has_explorer;
} node_manifest_t;

typedef struct {
  char *host;
  u16 port;
  char *public_host;
  bool allow_private_network;
} node_config_network_t;

typedef struct {
  bool enable;
  u64 rate_limiter_requests;
  u64 rate_limiter_duration;
} node_config_api_t;

typedef struct {
  char **bootstrap_peers;
  u64 bootstrap_peers_count;
} node_config_peers_t;

typedef struct {
  f64 max_resource_size;
  f64 limit;
} node_config_storage_t;

typedef struct {
  bool enable;
} node_config_explorer_t;

typedef struct {
  bool enable;
  char *id;
} node_config_update_t;

typedef struct {
  char *host;
  u16 port;
} node_config_db_t;

typedef struct {
  bool enable_updater;
  u32 updater_frequency;
} node_config_dht_t;

typedef struct {
  node_config_network_t network;
  node_config_db_t db;
  node_config_api_t api;
  node_config_peers_t peers;
  node_config_storage_t storage;
  node_config_explorer_t explorer;
  node_config_dht_t dht;
  node_config_update_t update;
} node_config_t;

typedef struct NODE_CTX {
  node_manifest_t *manifest;
  node_config_t *config;
  pid_t db_pid;
  pid_t node_pid;

  char *runtime_path;

  bool db_launched;

  bool node_server_started;
  bool should_exit;
  sonic_server_t *node_http_server;
} node_ctx_t;

result_t start_node_service(node_ctx_t *ctx, args_t *args, bool monitor);

result_t handle_node_session(args_t *args);

void shoggoth_node_exit(int exit_code);

#endif
