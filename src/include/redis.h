/**** redis.h ****
 *
 *  Copyright (c) 2023 Shoggoth Systems
 *
 * Part of the Shoggoth project, under the MIT License.
 * See LICENCE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#ifndef SHOG_REDIS
#define SHOG_REDIS

#include "../include/common.h"
#include "../utils/utils.h"

typedef enum {
  COM_GET,
  COM_SET,
  COM_WRITE,
  COM_APPEND,
  COM_DEL,
  COM_INCREMENT,
} command_type_t;

typedef struct {
  char *data;
  char *filter;
  char *com;
} redis_command_t;

redis_command_t gen_command(command_type_t com, char *filter, char *data);

result_t redis_write(char *host, u16 port, redis_command_t command);

result_t redis_read(char *host, u16 port, redis_command_t command);

#endif
