/**** netlibc.h ****
 *
 *  Copyright (c) 2023 ShogAI - https://shog.ai
 *
 * Part of netlibc, under the MIT License.
 * See LICENSE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#ifndef NETLIBC_H
#define NETLIBC_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef float f32;
typedef double f64;

typedef size_t usize;

#ifdef __APPLE__
#define U64_FORMAT_SPECIFIER "%llu"
#else
#define U64_FORMAT_SPECIFIER "%lu"
#endif

#ifdef __APPLE__
#define S64_FORMAT_SPECIFIER "%lld"
#else
#define S64_FORMAT_SPECIFIER "%ld"
#endif

#define LOOP()                                                                 \
  for (;;) {                                                                   \
  }

u64 random_number(u64 start, u64 end);

u64 get_timestamp_us();

u64 get_timestamp_ms();

u64 get_timestamp_s();

typedef struct {
  void *address;
  u64 size;
  bool freed;

  char *file;
  u64 line;
} mem_alloc_info_t;

typedef struct {
  bool mem_debug_enabled;

  mem_alloc_info_t *mem_allocations;
  u64 mem_allocations_count;
  u64 mem_free_count;
} netlibc_ctx_t;

extern netlibc_ctx_t *__netlibc_ctx;

void __netlibc_ctx_init(bool mem_debug_enabled);

void __netlibc_ctx_exit();

#ifndef MEM_DEBUG
#define MEM_DEBUG 0
#endif

#define NETLIBC_INIT() __netlibc_ctx_init(MEM_DEBUG)

#define ASSERT_NETLIBC_INIT()                                                  \
  do {                                                                         \
    if (__netlibc_ctx == NULL) {                                               \
      PANIC("NETLIBC NOT INITIALIZED");                                        \
    }                                                                          \
  } while (0)

#endif
