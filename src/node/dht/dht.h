/**** dht.h ****
 *
 *  Copyright (c) 2023 ShogAI - https://shog.ai
 *
 * Part of the Shoggoth project, under the MIT License.
 * See LICENSE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#ifndef SHOG_NODE_DHT_H
#define SHOG_NODE_DHT_H
#include "../node.h"
#include "../pins/pins.h"

typedef struct {
  // can be either a domain name or ip:port - if port is absent, use 80
  char *host;

  char *node_id;
  char *public_key;
  u64 unreachable_count;

  pins_t *pins;
} dht_item_t;

typedef struct DHT_T {
  dht_item_t **items;
  u64 items_count;
} dht_t;

typedef struct {
  char *peer_manifest;
  node_ctx_t *ctx;
} add_peer_thread_args_t;

typedef struct {
  node_ctx_t *ctx;
} dht_thread_args_t;

void free_dht_item(dht_item_t *item);

void free_dht(dht_t *dht);

dht_t *new_dht();

result_t dht_item_to_str(dht_item_t *item);

void dht_add_item(dht_t *dht, dht_item_t *item);

dht_item_t *new_dht_item();

result_t add_new_peer(node_ctx_t *ctx, char *peer_manifest);

void *dht_updater(void *thread_arg);

#endif
