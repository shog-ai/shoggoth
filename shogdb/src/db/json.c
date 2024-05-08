/**** json.c ****
 *
 *  Copyright (c) 2023 ShogAI - https://shog.ai
 *
 * Part of the ShogDB database, under the MIT License.
 * See LICENSE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#include "../../include/sonic.h"
#include "db.h"
#include "pins.h"

#include <netlibc/mem.h>

db_ctx_t *json_ctx = NULL;

cJSON *filter_json(cJSON *value, char *filter) {
  if (strcmp(filter, "$") == 0) {
    return value;
  }

  return value;
}

void json_append_route(sonic_server_request_t *req) {
  char *key = sonic_get_path_segment(req, "key");
  char *filter = sonic_get_path_segment(req, "filter");

  char *new_value = nmalloc((req->request_body_size + 1) * sizeof(char));
  memcpy(new_value, req->request_body, req->request_body_size);
  new_value[req->request_body_size] = '\0';

  result_t res = db_get_value(json_ctx, key);

  if (is_ok(res)) {
    db_value_t *value = VALUE(res);

    cJSON *filtered_value = filter_json(value->value_json, filter);

    cJSON *new_value_json = cJSON_Parse(new_value);
    nfree(new_value);

    cJSON_AddItemToArray(filtered_value, new_value_json);

    sonic_server_response_t *resp =
        sonic_new_response(STATUS_200, MIME_TEXT_PLAIN);

    char *body = "OK";
    sonic_response_set_body(resp, body, strlen(body));
    sonic_send_response(req, resp);

    sonic_free_server_response(resp);

    json_ctx->saved = false;

    return;
  } else {
    sonic_server_response_t *resp =
        sonic_new_response(STATUS_406, MIME_TEXT_PLAIN);

    char err[256];
    sprintf(err, "ERR %s", res.error_message);

    sonic_response_set_body(resp, err, strlen(err));
    sonic_send_response(req, resp);
    sonic_free_server_response(resp);

    return;
  }
}

void json_get_route(sonic_server_request_t *req) {
  char *key = sonic_get_path_segment(req, "key");
  char *filter = sonic_get_path_segment(req, "filter");

  result_t res = db_get_value(json_ctx, key);

  if (is_ok(res)) {
    db_value_t *value = VALUE(res);

    cJSON *filtered_value = filter_json(value->value_json, filter);

    char *str = cJSON_Print(filtered_value);
    char *body = string_from("JSON", " ", str, NULL);
    free(str);

    sonic_server_response_t *resp =
        sonic_new_response(STATUS_200, MIME_TEXT_PLAIN);

    sonic_response_set_body(resp, body, strlen(body));
    sonic_send_response(req, resp);
    nfree(body);

    sonic_free_server_response(resp);

    return;
  } else {
    sonic_server_response_t *resp =
        sonic_new_response(STATUS_406, MIME_TEXT_PLAIN);

    char err[256];
    sprintf(err, "ERR %s", res.error_message);

    sonic_response_set_body(resp, err, strlen(err));
    sonic_send_response(req, resp);
    sonic_free_server_response(resp);

    return;
  }
}

void add_json_routes(db_ctx_t *ctx, sonic_server_t *server) {
  json_ctx = ctx;

  sonic_add_route(server, "/json_append/{key}/{filter}", METHOD_GET,
                  json_append_route);

  sonic_add_route(server, "/json_get/{key}/{filter}", METHOD_GET,
                  json_get_route);
}
