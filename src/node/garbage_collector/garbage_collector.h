/**** garbage_collector.h ****
 *
 *  Copyright (c) 2023 Shoggoth Systems
 *
 * Part of the Shoggoth project, under the MIT License.
 * See LICENCE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#ifndef SHOG_NODE_GARBAGE_COLLECTOR_H
#define SHOG_NODE_GARBAGE_COLLECTOR_H

#include "../../include/common.h"
#include "../node.h"

typedef struct {
  node_ctx_t *ctx;
} garbage_collector_thread_args_t;

void *garbage_collector(void *thread_arg);

#endif