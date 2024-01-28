/**** rate_limiter.c ****
 *
 *  Copyright (c) 2023 ShogAI - https://shog.ai
 *
 * Part of the Sonic HTTP library, under the MIT License.
 * See LICENSE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#define _GNU_SOURCE

#include "rate_limiter.h"
#include "../../sonic.h"

#include <stdlib.h>
#include <unistd.h>

#include <assert.h>
#include <pthread.h>
#include <sys/time.h>

// returns the UNIX timestamp in microseconds
u64 timestamp_us() {
  struct timeval tv;
  gettimeofday(&tv, NULL);

  return (u64)tv.tv_sec * 1000000L + (unsigned long)tv.tv_usec;
}

// returns the UNIX timestamp in milliseconds
u64 timestamp_ms() {
  u64 us = timestamp_us();
  return us / 1000; // Convert microseconds to milliseconds
}

// returns the UNIX timestamp in seconds
u64 timestamp_s() {
  u64 ms = timestamp_ms();
  return ms / 1000; // Convert milliseconds to seconds
}

void rate_limiter_add_client(rate_limiter_t *limiter,
                             rate_limiter_client_t *client) {
  limiter->clients =
      realloc(limiter->clients,
              (limiter->clients_count + 1) * sizeof(rate_limiter_client_t *));

  limiter->clients[limiter->clients_count] = client;

  limiter->clients_count++;
}

void free_rate_limiter_client(rate_limiter_client_t *client) {
  free(client->ip_address);

  free(client);
}

void rate_limiter_remove_client(rate_limiter_t *limiter, u64 index) {
  free_rate_limiter_client(limiter->clients[index]);

  for (u64 i = index + 1; i < limiter->clients_count; i++) {
    limiter->clients[i - 1] = limiter->clients[i];
  }

  limiter->clients =
      realloc(limiter->clients,
              (limiter->clients_count - 1) * sizeof(rate_limiter_client_t *));

  limiter->clients_count--;
}

bool rate_limiter_client_exists(rate_limiter_t *limiter, char *ip_address) {

  for (u64 i = 0; i < limiter->clients_count; i++) {
    if (strcmp(limiter->clients[i]->ip_address, ip_address) == 0) {
      return true;
    }
  }

  return false;
}

rate_limiter_client_t *rate_limiter_get_client(rate_limiter_t *limiter,
                                               char *ip_address) {

  for (u64 i = 0; i < limiter->clients_count; i++) {
    if (strcmp(limiter->clients[i]->ip_address, ip_address) == 0) {
      return limiter->clients[i];
    }
  }

  return NULL;
}

rate_limiter_t *new_rate_limiter(u64 requests_count, u64 duration,
                                 char *message) {
  rate_limiter_t *rate_limiter = calloc(1, sizeof(rate_limiter_t));
  rate_limiter->clients = NULL;
  rate_limiter->clients_count = 0;
  rate_limiter->message = message;
  rate_limiter->max_tokens = (f64)requests_count;
  rate_limiter->refill_rate = (f64)duration;

  return rate_limiter;
}

void free_rate_limiter(rate_limiter_t *rate_limiter) {
  if (rate_limiter->clients_count > 0) {
    for (u64 i = 0; i < rate_limiter->clients_count; i++) {
      free_rate_limiter_client(rate_limiter->clients[i]);
    }

    rate_limiter->clients_count = 0;
  }

  free(rate_limiter);
}

token_bucket_t *new_token_bucket(f64 max_tokens, f64 refill_rate) {
  token_bucket_t *bucket = malloc(sizeof(token_bucket_t));

  bucket->tokens = max_tokens;
  bucket->max_tokens = max_tokens;
  bucket->refill_rate = refill_rate;
  bucket->last_refill_time = timestamp_ms();

  return bucket;
}

void refill_token_bucket(token_bucket_t *bucket) {
  u64 now = timestamp_ms();
  f64 duration = (f64)now - bucket->last_refill_time;

  f64 tokens_to_add = (bucket->max_tokens * duration) / bucket->refill_rate;

  bucket->tokens = (bucket->tokens + tokens_to_add) < bucket->max_tokens
                       ? (bucket->tokens + tokens_to_add)
                       : bucket->max_tokens;

  bucket->last_refill_time = now;
}

bool token_bucket_request(token_bucket_t *bucket, f64 tokens) {
  refill_token_bucket(bucket);

  if (tokens < bucket->tokens) {
    bucket->tokens -= tokens;
    return true;
  }

  return false;
}

rate_limiter_client_t *new_rate_limiter_client(char *ip_address, f64 max_tokens,
                                               f64 refill_rate) {
  rate_limiter_client_t *new_client = malloc(sizeof(rate_limiter_client_t));

  new_client->token_bucket = new_token_bucket(max_tokens, refill_rate);
  new_client->ip_address = strdup(ip_address);

  return new_client;
}

sonic_middleware_response_t *
rate_limiter_middleware_function(sonic_server_request_t *req,
                                 void *user_pointer) {
  rate_limiter_t *rate_limiter = (rate_limiter_t *)user_pointer;

  if (!rate_limiter_client_exists(rate_limiter, req->client_ip)) {

    rate_limiter_client_t *new_client = new_rate_limiter_client(
        req->client_ip, rate_limiter->max_tokens, rate_limiter->refill_rate);

    rate_limiter_add_client(rate_limiter, new_client);
  }

  rate_limiter_client_t *rate_limiter_client =
      rate_limiter_get_client(rate_limiter, req->client_ip);
  assert(rate_limiter_client != NULL);

  rate_limiter_client->last_request_time = timestamp_ms();

  bool allow = token_bucket_request(rate_limiter_client->token_bucket, 1);

  if (allow) {
    sonic_middleware_response_t *response =
        sonic_new_middleware_response(false, NULL);

    return response;
  } else {
    sonic_server_response_t *resp =
        sonic_new_response(STATUS_429, MIME_TEXT_PLAIN);

    char *body = rate_limiter->message;

    sonic_response_set_body(resp, body, strlen(body));

    sonic_middleware_response_t *response =
        sonic_new_middleware_response(true, resp);

    return response;
  }
}

void *rate_limiter_background(void *thread_arg) {
  pthread_detach(pthread_self());

  rate_limiter_t *rate_limiter = (rate_limiter_t *)thread_arg;

  for (;;) {
    sleep(5);

    for (u64 i = 0; i < rate_limiter->clients_count; i++) {
      u64 now = timestamp_ms();

      u64 cleanup_timeout = 5000; // 5 seconds

      if ((now - rate_limiter->clients[i]->last_request_time) >
          cleanup_timeout) {
        rate_limiter_remove_client(rate_limiter, i);
      }
    }
  }

  return NULL;
}
