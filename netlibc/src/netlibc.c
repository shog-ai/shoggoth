/**** netlibc.c ****
 *
 *  Copyright (c) 2023 ShogAI - https://shog.ai
 *
 * Part of netlibc, under the MIT License.
 * See LICENSE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#include "../include/netlibc.h"
#include "../include/netlibc/log.h"
#include "../include/netlibc/mem.h"
#include "../include/netlibc/string.h"

#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <uuid/uuid.h>

netlibc_ctx_t *__netlibc_ctx = NULL;

void __netlibc_ctx_init(bool mem_debug_enabled) {
  __netlibc_ctx = calloc(1, sizeof(netlibc_ctx_t));

  __netlibc_ctx->mem_debug_enabled = mem_debug_enabled;
  __netlibc_ctx->mem_allocations = NULL;
  __netlibc_ctx->mem_allocations_count = 0;
  __netlibc_ctx->mem_free_count = 0;

  // Register the cleanup_function
  if (atexit(__netlibc_ctx_exit) != 0) {
    PANIC("atexit registration failed");
  }
}

void __netlibc_ctx_exit() {
  if (__netlibc_ctx->mem_debug_enabled) {
    _mem_summary();
  }

  free(__netlibc_ctx->mem_allocations);

  free(__netlibc_ctx);
}

// returns the UNIX timestamp in microseconds
u64 get_timestamp_us() {
  struct timeval tv;
  gettimeofday(&tv, NULL);

  return (u64)tv.tv_sec * 1000000L + (unsigned long)tv.tv_usec;
}

// returns the UNIX timestamp in milliseconds
u64 get_timestamp_ms() {
  u64 us = get_timestamp_us();
  return us / 1000; // Convert microseconds to milliseconds
}

// returns the UNIX timestamp in seconds
u64 get_timestamp_s() {
  u64 ms = get_timestamp_ms();
  return ms / 1000; // Convert milliseconds to seconds
}

/****
 * extracts only the filename from a file path excluding the extension
 *
 ****/
result_t remove_file_extension(char *str) {
  // Find the last dot (.) in the string
  const char *lastDot = strrchr(str, '.');

  if (lastDot == NULL) {
    // If no dot is found, return a copy of the original string
    return OK(strdup(str));
  } else {
    // Calculate the length of the new string without the extension
    size_t newLength = (size_t)(lastDot - str);

    // Allocate memory for the new string
    char *newStr = (char *)malloc(newLength + 1);

    if (newStr != NULL) {
      // Copy the characters from the original string to the new string
      strncpy(newStr, str, newLength);
      newStr[newLength] = '\0'; // Null-terminate the new string
    }

    return OK(newStr);
  }
}

u64 random_number(u64 start, u64 end) {
  if (start >= end) {
    // Invalid input, return 0 or handle the error as needed.
    return 0;
  }

  // Initialize the random number generator with the current time.
  srand((u32)time(NULL));

  // Generate a random number within the specified range.
  u64 random_num = (u64)start + (u64)rand() % (u64)(end - start + 1);

  return random_num;
}

#ifdef __APPLE__
#define UUID_STR_LEN 37
#endif

char *gen_uuid() {
  uuid_t binuuid;
  uuid_generate_random(binuuid);

  char *uuid = nmalloc(UUID_STR_LEN * sizeof(char));

  /*
   * Produces a UUID string at uuid consisting of letters
   * whose case depends on the system's locale.
   */
  uuid_unparse(binuuid, uuid);

  return uuid;
}
