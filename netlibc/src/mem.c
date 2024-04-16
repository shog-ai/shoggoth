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

void *__netlibc_realloc(void *pre_allocated, size_t new_size, char *file,
                        u64 line) {
  ASSERT_NETLIBC_INIT();

  void *reallocated = realloc(pre_allocated, new_size);

  if (__netlibc_ctx->mem_debug_enabled) {
    if (pre_allocated == NULL) {
      mem_add_alloation(pre_allocated, new_size, file, line);
    }

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
        break;
      }

      if (i == (__netlibc_ctx->mem_allocations_count - 1)) {
        // printf("[MEM_DEBUG] free of unallocated pointer (%p) detected at "
        // "%s:" U64_FORMAT_SPECIFIER "\n\n",
        // allocated, file, line);
        // abort();
      }
    }

    for (u64 i = 0; i < __netlibc_ctx->mem_allocations_count; i++) {
      if (__netlibc_ctx->mem_allocations[i].address == allocated) {
        if (__netlibc_ctx->mem_allocations[i].freed) {
          // printf("[MEM_DEBUG] double free detected at %s:"
          // U64_FORMAT_SPECIFIER
          // ". previously freed at %s:" U64_FORMAT_SPECIFIER "\n\n",
          // file, line, __netlibc_ctx->mem_allocations[i].freed_file,
          // __netlibc_ctx->mem_allocations[i].freed_line);
          // abort();
        }

        __netlibc_ctx->mem_allocations[i].freed = true;
        __netlibc_ctx->mem_allocations[i].freed_file = file;
        __netlibc_ctx->mem_allocations[i].freed_line = line;
        __netlibc_ctx->mem_free_count += 1;
      }
    }
  }

  free(allocated);
}

void _mem_summary() {
  u64 allocated_bytes = 0;
  for (u64 i = 0; i < __netlibc_ctx->mem_allocations_count; i++) {
    allocated_bytes += __netlibc_ctx->mem_allocations[i].size;
  }

  u64 freed_bytes = 0;
  for (u64 i = 0; i < __netlibc_ctx->mem_allocations_count; i++) {
    if (__netlibc_ctx->mem_allocations[i].freed) {
      freed_bytes += __netlibc_ctx->mem_allocations[i].size;
    }
  }

  u64 leaked_bytes = 0;
  u64 leaked_allocations = 0;
  for (u64 i = 0; i < __netlibc_ctx->mem_allocations_count; i++) {
    if (!__netlibc_ctx->mem_allocations[i].freed) {
      leaked_bytes += __netlibc_ctx->mem_allocations[i].size;
      leaked_allocations += 1;

      printf("[MEM_DEBUG] Direct leak of " U64_FORMAT_SPECIFIER
             " bytes (%p) allocated from %s:" U64_FORMAT_SPECIFIER "\n\n",
             __netlibc_ctx->mem_allocations[i].size,
             __netlibc_ctx->mem_allocations[i].address,
             __netlibc_ctx->mem_allocations[i].file,
             __netlibc_ctx->mem_allocations[i].line);
    }
  }

  printf("[MEM_DEBUG] Total allocated bytes: " U64_FORMAT_SPECIFIER " \n",
         allocated_bytes);

  printf("[MEM_DEBUG] Total freed bytes: " U64_FORMAT_SPECIFIER " \n",
         freed_bytes);

  printf("[MEM_DEBUG] allocations: " U64_FORMAT_SPECIFIER " \n",
         __netlibc_ctx->mem_allocations_count);

  printf("[MEM_DEBUG] frees: " U64_FORMAT_SPECIFIER " \n",
         __netlibc_ctx->mem_free_count);

  printf("\n");

  if (leaked_bytes > 0) {
    printf("[MEM_DEBUG] leaked " U64_FORMAT_SPECIFIER
           " bytes in " U64_FORMAT_SPECIFIER " allocations\n",
           leaked_bytes, leaked_allocations);
    // abort();
  } else {
    printf("[MEM_DEBUG] All memory freed");
  }

  printf("\n\n");
}
