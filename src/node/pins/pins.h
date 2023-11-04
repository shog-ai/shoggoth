/**** pins.h ****
 *
 *  Copyright (c) 2023 Shoggoth Systems
 *
 * Part of the Shoggoth project, under the MIT License.
 * See LICENCE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#ifndef SHOG_NODE_PINS_H
#define SHOG_NODE_PINS_H

#include "../../include/common.h"
#include "../node.h"

typedef struct {
  char *shoggoth_id;
  char **pinning_nodes;
} pin_t;

typedef struct {
  char **pins;
  u64 pins_count;
} pins_t;

typedef struct {
  node_ctx_t *ctx;
} pins_downloader_thread_args_t;

typedef struct {
  node_ctx_t *ctx;
} pins_updater_thread_args_t;

void free_pins(pins_t *pins);

void *pins_updater(void *thread_arg);

void *pins_downloader(void *thread_arg);

#endif
