/**** json.c ****
 *
 *  Copyright (c) 2023 ShogAI - https://shog.ai
 *
 * Part of the Shoggoth project, under the MIT License.
 * See LICENSE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#include "../include/cjson.h"
#include "../studio.h"
#include "../utils/utils.h"

#include "json.h"

#include <stdlib.h>

#include <netlibc/mem.h>

void free_json(json_t *json) {
  cJSON *cjson = (cJSON *)json;

  cJSON_Delete(cjson);
}

char *json_to_string(json_t *json) {
  cJSON *cjson = (cJSON *)json;

  return cJSON_Print(cjson);
}

result_t json_to_studio_model(json_t *json) {
  cJSON *model_json = (cJSON *)json;

  studio_model_t *model = new_studio_model();

  model->name =
      strdup(cJSON_GetObjectItemCaseSensitive(model_json, "name")->valuestring);

  return OK(model);
}

result_t json_to_studio_active_model(json_t *json) {
  cJSON *model_json = (cJSON *)json;

  studio_active_model_t *model = new_studio_active_model();

  model->name =
      strdup(cJSON_GetObjectItemCaseSensitive(model_json, "name")->valuestring);

  model->status = "unmounted";

  return OK(model);
}

result_t json_to_studio_models(json_t *json) {
  cJSON *models_json = (cJSON *)json;

  studio_models_t *models = new_studio_models();

  const cJSON *model_json = NULL;
  cJSON_ArrayForEach(models_json, model_json) {
    result_t res_new_item = json_to_studio_model((void *)model_json);
    studio_model_t *new_item = PROPAGATE(res_new_item);

    studio_models_add_model(models, new_item);
  }

  return OK(models);
}

result_t json_studio_model_to_json(studio_model_t model) {
  cJSON *model_json = cJSON_CreateObject();

  cJSON_AddStringToObject(model_json, "name", model.name);

  return OK(model_json);
}

result_t json_studio_active_model_to_json(studio_active_model_t model) {
  cJSON *model_json = cJSON_CreateObject();

  cJSON_AddStringToObject(model_json, "name", model.name);
  cJSON_AddStringToObject(model_json, "status", model.status);

  return OK(model_json);
}

result_t json_studio_models_to_json(studio_models_t models) {
  cJSON *models_json = cJSON_CreateArray();

  for (u64 i = 0; i < models.items_count; i++) {
    result_t res_item_json = json_studio_model_to_json(*models.items[i]);
    cJSON *item_json = (cJSON *)PROPAGATE(res_item_json);

    cJSON_AddItemToArray(models_json, item_json);
  }

  return OK(models_json);
}

result_t json_studio_state_to_json(studio_state_t state) {
  cJSON *state_json = cJSON_CreateObject();

  result_t res_models_json = json_studio_models_to_json(*state.models);
  cJSON *models_json = PROPAGATE(res_models_json);
  cJSON_AddItemToObject(state_json, "models", models_json);

  result_t res_active_model_json =
      json_studio_active_model_to_json(*state.active_model);
  cJSON *active_model_json = PROPAGATE(res_active_model_json);
  cJSON_AddItemToObject(state_json, "active_model", active_model_json);

  return OK(state_json);
}

result_t json_to_completion_request(json_t *json) {
  cJSON *req_json = (cJSON *)json;

  completion_request_t *req = new_completion_request();

  cJSON *prompt = cJSON_GetObjectItemCaseSensitive(req_json, "prompt");
  if (prompt == NULL) {
    return ERR("prompt object not found");
  }
  req->prompt = strdup(prompt->valuestring);

  return OK(req);
}

result_t json_string_to_completion_request(char *str) {
  cJSON *req_json = cJSON_Parse(str);
  if (req_json == NULL) {
    return ERR("Could not parse completion request json: %s\n", str);
  }

  result_t res_req = json_to_completion_request((void *)req_json);
  completion_request_t *req = PROPAGATE(res_req);

  cJSON_Delete(req_json);

  return OK(req);
}
