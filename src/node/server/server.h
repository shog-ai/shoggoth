/**** server.h ****
 *
 *  Copyright (c) 2023 Shoggoth Systems
 *
 * Part of the Shoggoth project, under the MIT License.
 * See LICENSE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#ifndef SERVER_H
#define SERVER_H

#include <netlibc.h>
#include <netlibc/error.h>
#include <netlibc/fs.h>
#include <netlibc/log.h>
#include <netlibc/string.h>

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
