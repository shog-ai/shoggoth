/**** netlibc.h ****
 *
 *  Copyright (c) 2023 Shoggoth Systems - https://shoggoth.systems
 *
 * Part of netlibc, under the MIT License.
 * See LICENCE file for license information.
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
#define U64_FORMAT_SPECIFIER "%lld"
#else
#define U64_FORMAT_SPECIFIER "%lu"
#endif

#define LOOP()                                                                 \
  for (;;) {                                                                   \
  }

// struct NODE_CTX;
// struct CLIENT_CTX;

// #define SERVER_ERR(res)                                                        \
//   do {                                                                         \
//     if (is_err(res)) {                                                         \
//       respond_error(req, res.error_message);                                   \
//       free_result(res);                                                        \
//       return;                                                                  \
//     }                                                                          \
//                                                                                \
//     free_result(res);                                                          \
//   } while (0)

u64 random_number(u64 start, u64 end);

u64 get_timestamp_us();

u64 get_timestamp_ms();

u64 get_timestamp_s();

#endif