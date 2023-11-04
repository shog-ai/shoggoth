/**** common.h ********************
 *
 *  Copyright (c) 2023 Netrunner KD6-3.7
 *
 * Part of the Camel testing framework, under the MIT License.
 * See LICENCE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****************************************************************/

#ifndef KIM_COMMON_H
#define KIM_COMMON_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

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

#define LOOP()                                                                 \
  for (;;) {                                                                   \
  }

#ifdef __APPLE__ // This macro is defined on macOS
#define U64_FORMAT_SPECIFIER "%lld"
#else
#define U64_FORMAT_SPECIFIER "%lu"
#endif

#endif