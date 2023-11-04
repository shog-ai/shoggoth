/**** rate_limiter.h ****
 *
 *  Copyright (c) 2023 Shoggoth Systems - https://shoggoth.systems
 *
 * Part of the Sonic HTTP library, under the MIT License.
 * See LICENCE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#ifndef SHOG_SERVER_RATE_LIMITER
#define SHOG_SERVER_RATE_LIMITER

#include "../../sonic.h"

typedef struct {
  f64 tokens;
  f64 max_tokens;
  f64 refill_rate;

  u64 last_refill_time;
} token_bucket_t;

typedef struct {
  char *ip_address;
  token_bucket_t *token_bucket;
  u64 last_request_time;
} rate_limiter_client_t;

typedef struct {
  char *message;

  rate_limiter_client_t **clients;
  u64 clients_count;

  f64 max_tokens;
  f64 refill_rate;
} rate_limiter_t;

rate_limiter_t *new_rate_limiter(u64 requests_count, u64 duration,
                                 char *message);

void free_rate_limiter(rate_limiter_t *rate_limiter);

sonic_middleware_response_t *
rate_limiter_middleware_function(sonic_server_request_t *req,
                                 void *user_pointer);

void *rate_limiter_background(void *thread_arg);

#endif