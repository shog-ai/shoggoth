/**** utils.c ****
 *
 *  Copyright (c) 2023 ShogAI - https://shog.ai
 *
 * Part of the Shoggoth project, under the MIT License.
 * See LICENSE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#include "utils.h"
// #include "../include/tuwi.h"
// #include "../node/node.h"
#include "../studio.h"

#include <assert.h>
#include <stdlib.h>
#include <uuid/uuid.h>

#include <netlibc/fs.h>
#include <netlibc/mem.h>

#ifdef __APPLE__
#define UUID_STR_LEN 37
#endif

void utils_verify_studio_runtime_dirs(studio_ctx_t *ctx) {
  char runtime_path[256];
  strcpy(runtime_path, ctx->runtime_path);

  char studio_runtime_path[256];
  sprintf(studio_runtime_path, "%s/studio", runtime_path);

  char models_path[256];
  sprintf(models_path, "%s/models", studio_runtime_path);

  if (!dir_exists(runtime_path)) {
    create_dir(runtime_path);
  }

  if (!dir_exists(studio_runtime_path)) {
    create_dir(studio_runtime_path);
  }

  if (!dir_exists(models_path)) {
    create_dir(models_path);
  }
}

// /****
//  * utility function to get the default runtime path which should be
//  * $HOME/shoggoth
//  *
//  ****/
int utils_get_default_runtime_path(char runtime_path[]) {
#ifdef _WIN32
  const char *home_path = getenv("USERPROFILE");
#else
  const char *home_path = getenv("HOME");
#endif

  if (home_path == NULL) {
    return 1;
  }

  char *sub_dir = "/shogstudio";
  sprintf(runtime_path, "%s%s", home_path, sub_dir);

  return 0;
}
