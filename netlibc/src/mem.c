/**** mem.c ****
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

#include <stdlib.h>

void mem_add_alloation(void *address, size_t size, char *file, u64 line) {
  ASSERT_NETLIBC_INIT();

  __netlibc_ctx->mem_allocations = realloc(
      __netlibc_ctx->mem_allocations,
      (__netlibc_ctx->mem_allocations_count + 1) * sizeof(mem_alloc_info_t));

  __netlibc_ctx->mem_allocations[__netlibc_ctx->mem_allocations_count].address =
      address;

  __netlibc_ctx->mem_allocations[__netlibc_ctx->mem_allocations_count].size =
      size;

  __netlibc_ctx->mem_allocations[__netlibc_ctx->mem_allocations_count].freed =
      false;

  __netlibc_ctx->mem_allocations[__netlibc_ctx->mem_allocations_count].file =
      file;

  __netlibc_ctx->mem_allocations[__netlibc_ctx->mem_allocations_count].line =
      line;

  __netlibc_ctx->mem_allocations_count++;
}

void *__netlibc_malloc(size_t size, char *file, u64 line) {
  ASSERT_NETLIBC_INIT();

  void *allocated = malloc(size);

  if (__netlibc_ctx->mem_debug_enabled) {
    mem_add_alloation(allocated, size, file, line);
  }

  return allocated;
}

void *__netlibc_calloc(u64 count, size_t size, char *file, u64 line) {
  ASSERT_NETLIBC_INIT();

  void *allocated = calloc(count, size);

  if (__netlibc_ctx->mem_debug_enabled) {
    mem_add_alloation(allocated, size * count, file, line);
  }

  return allocated;
}

void *__netlibc_realloc(void *pre_allocated, size_t new_size) {
  ASSERT_NETLIBC_INIT();

  void *reallocated = realloc(pre_allocated, new_size);

  if (__netlibc_ctx->mem_debug_enabled) {
    for (u64 i = 0; i < __netlibc_ctx->mem_allocations_count; i++) {
      if (__netlibc_ctx->mem_allocations[i].address == pre_allocated) {
        __netlibc_ctx->mem_allocations[i].address = reallocated;
        __netlibc_ctx->mem_allocations[i].size = new_size;
      }
    }
  }

  return reallocated;
}

void __netlibc_free(void *allocated, char *file, u64 line) {
  ASSERT_NETLIBC_INIT();

  if (__netlibc_ctx->mem_debug_enabled) {
    for (u64 i = 0; i < __netlibc_ctx->mem_allocations_count; i++) {
      if (__netlibc_ctx->mem_allocations[i].address == allocated) {
        if (__netlibc_ctx->mem_allocations[i].freed) {
          printf("[MEM_DEBUG] DOUBLE FREE DETECTED AT %s:" U64_FORMAT_SPECIFIER
                 "\n",
                 file, line);
        }

        __netlibc_ctx->mem_allocations[i].freed = true;
        __netlibc_ctx->mem_free_count += 1;
      }
    }
  }

  free(allocated);
}
