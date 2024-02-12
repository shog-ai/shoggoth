/**** pins.h ****
 *
 *  Copyright (c) 2023 ShogAI - https://shog.ai
 *
 * Part of the Shoggoth project, under the MIT License.
 * See LICENSE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#ifndef SHOG_NODE_PINS_H
#define SHOG_NODE_PINS_H

#include <netlibc.h>
#include <netlibc/error.h>
#include <netlibc/fs.h>
#include <netlibc/log.h>
#include <netlibc/string.h>

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
