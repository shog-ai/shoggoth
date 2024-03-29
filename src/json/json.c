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
#include "../node/dht/dht.h"
#include "../node/server/server.h"
#include "../studio/studio.h"
#include "../utils/utils.h"

#include "json.h"

#include <stdlib.h>

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

result_t json_dht_item_to_json(dht_item_t item) {
  cJSON *item_json = cJSON_CreateObject();

  cJSON_AddStringToObject(item_json, "host", item.host);

  cJSON_AddStringToObject(item_json, "node_id", item.node_id);

  cJSON_AddStringToObject(item_json, "public_key", item.public_key);

  cJSON_AddNumberToObject(item_json, "unreachable_count",
                          (int)item.unreachable_count);

  cJSON *pins_json = cJSON_CreateArray();

  for (u64 k = 0; k < item.pins->pins_count; k++) {
    cJSON *pin_json = cJSON_CreateString(item.pins->pins[k]);

    cJSON_AddItemToArray(pins_json, pin_json);
  }

  cJSON_AddItemToObject(item_json, "pins", pins_json);

  return OK(item_json);
}

result_t json_dht_to_json(dht_t dht) {
  cJSON *items_json = cJSON_CreateArray();

  for (u64 i = 0; i < dht.items_count; i++) {
    result_t res_item_json = json_dht_item_to_json(*dht.items[i]);
    cJSON *item_json = (cJSON *)PROPAGATE(res_item_json);

    cJSON_AddItemToArray(items_json, item_json);
  }

  return OK(items_json);
}

result_t json_node_manifest_to_json(node_manifest_t manifest) {
  cJSON *manifest_json = cJSON_CreateObject();

  cJSON_AddStringToObject(manifest_json, "public_host", manifest.public_host);

  cJSON_AddStringToObject(manifest_json, "public_key", manifest.public_key);

  cJSON_AddStringToObject(manifest_json, "node_id", manifest.node_id);

  cJSON_AddStringToObject(manifest_json, "version", manifest.version);

  cJSON_AddBoolToObject(manifest_json, "has_explorer", manifest.has_explorer);

  return OK(manifest_json);
}

pins_t *json_to_pins(json_t *json) {
  cJSON *pins_json = (cJSON *)json;

  pins_t *pins = calloc(1, sizeof(pins_t));
  pins->pins = NULL;
  pins->pins_count = 0;

  const cJSON *pin_json = NULL;
  cJSON_ArrayForEach(pin_json, pins_json) {
    char *pin_str = pin_json->valuestring;

    pins->pins = realloc(pins->pins, (pins->pins_count + 1) * sizeof(char *));
    pins->pins[pins->pins_count] = strdup(pin_str);
    pins->pins_count++;
  }

  return pins;
}

result_t json_to_dht_item(json_t *json) {
  cJSON *dht_item_json = (cJSON *)json;

  dht_item_t *dht_item = new_dht_item();

  dht_item->host = strdup(
      cJSON_GetObjectItemCaseSensitive(dht_item_json, "host")->valuestring);

  dht_item->node_id = strdup(
      cJSON_GetObjectItemCaseSensitive(dht_item_json, "node_id")->valuestring);

  dht_item->public_key =
      strdup(cJSON_GetObjectItemCaseSensitive(dht_item_json, "public_key")
                 ->valuestring);

  dht_item->unreachable_count =
      (u64)cJSON_GetObjectItemCaseSensitive(dht_item_json, "unreachable_count")
          ->valueint;

  cJSON *pins_json = cJSON_GetObjectItemCaseSensitive(dht_item_json, "pins");

  dht_item->pins = json_to_pins((void *)pins_json);

  return OK(dht_item);
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

result_t json_to_dht(json_t *json) {
  cJSON *dht_json = (cJSON *)json;

  dht_t *dht = new_dht();

  const cJSON *dht_item_json = NULL;
  cJSON_ArrayForEach(dht_item_json, dht_json) {
    result_t res_new_item = json_to_dht_item((void *)dht_item_json);
    dht_item_t *new_item = PROPAGATE(res_new_item);

    dht_add_item(dht, new_item);
  }

  return OK(dht);
}

result_t json_string_to_dht(char *dht_str) {
  cJSON *dht_json = cJSON_Parse(dht_str);
  if (dht_json == NULL) {
    return ERR("Could not parse dht json \n");
  }

  result_t res_dht = json_to_dht((void *)dht_json);
  dht_t *dht = PROPAGATE(res_dht);

  cJSON_Delete(dht_json);

  return OK(dht);
}

result_t json_string_to_pins(char *pins_str) {
  cJSON *pins_json = cJSON_Parse(pins_str);
  if (pins_json == NULL) {
    return ERR("Could not parse pins json \n");
  }

  pins_t *pins = json_to_pins((void *)pins_json);

  cJSON_Delete(pins_json);

  return OK(pins);
}

node_manifest_t *json_to_node_manifest(json_t *json) {
  cJSON *manifest_json = (cJSON *)json;

  char *public_host =
      strdup(cJSON_GetObjectItemCaseSensitive(manifest_json, "public_host")
                 ->valuestring);

  char *public_key =
      strdup(cJSON_GetObjectItemCaseSensitive(manifest_json, "public_key")
                 ->valuestring);

  char *node_id = strdup(
      cJSON_GetObjectItemCaseSensitive(manifest_json, "node_id")->valuestring);

  char *version = strdup(
      cJSON_GetObjectItemCaseSensitive(manifest_json, "version")->valuestring);

  bool has_explorer =
      cJSON_GetObjectItemCaseSensitive(manifest_json, "has_explorer")->valueint;

  node_manifest_t *manifest = calloc(1, sizeof(node_manifest_t));

  manifest->public_host = public_host;
  manifest->public_key = public_key;
  manifest->node_id = node_id;
  manifest->version = version;
  manifest->has_explorer = has_explorer;

  return manifest;
}

result_t json_string_to_node_manifest(char *manifest_str) {
  cJSON *manifest_json = cJSON_Parse(manifest_str);
  if (manifest_json == NULL) {
    return ERR("Could not parse manifest json: %s\n", manifest_str);
  }

  node_manifest_t *manifest = json_to_node_manifest((void *)manifest_json);

  cJSON_Delete(manifest_json);

  return OK(manifest);
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
