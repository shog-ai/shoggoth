/**** server.h ****
 *
 *  Copyright (c) 2023 Shoggoth Systems
 *
 * Part of the Shoggoth project, under the MIT License.
 * See LICENCE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#ifndef SERVER_H
#define SERVER_H

#include <netlibc/netlibc.h>
#include "../node.h"

typedef struct {
  char *shoggoth_id;
  u64 upload_size;
  u64 chunk_size_limit;
  u64 chunk_count;
} upload_info_t;

typedef struct {
  node_ctx_t *ctx;
} server_thread_args_t;

void *start_node_server(void *thread_args);

#endif
