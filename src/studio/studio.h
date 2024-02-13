/**** studio.h ****
 *
 *  Copyright (c) 2023 ShogAI
 *
 * Part of the Shoggoth project, under the MIT License.
 * See LICENSE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#ifndef SHOG_STUDIO_H
#define SHOG_STUDIO_H

#include "../args/args.h"

typedef struct STUDIO_CTX {
  char *runtime_path;
} studio_ctx_t;

result_t start_studio(args_t *args);

#endif
