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

typedef struct {
  char *name;
} studio_model_t;

typedef struct {
  studio_model_t **items;
  u64 items_count;
} studio_models_t;

typedef struct {
  studio_models_t *models;
  
  studio_model_t *active_model;
  bool model_mounted;
} studio_state_t;

typedef struct STUDIO_CTX {
  char *runtime_path;
  studio_state_t *state;
} studio_ctx_t;

studio_model_t *new_studio_model();

void studio_models_add_model(studio_models_t *models, studio_model_t *item);

studio_models_t *new_studio_models();

result_t start_studio(args_t *args);

#endif
