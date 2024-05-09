/**** dht.c ****
 *
 *  Copyright (c) 2023 ShogAI - https://shog.ai
 *
 * Part of the ShogDB database, under the MIT License.
 * See LICENSE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#include "dht.h"
#include "../../include/sonic.h"
#include "db.h"

db_ctx_t *dht_ctx = NULL;

void dht_peer_clear_pins_route(sonic_server_request_t *req) {
  result_t res = db_get_value(dht_ctx, "dht");

  if (is_ok(res)) {
    db_value_t *value = VALUE(res);
    cJSON *dht = value->value_json;

    char *req_body = malloc((req->request_body_size + 1) * sizeof(char));
    strncpy(req_body, req->request_body, req->request_body_size);
    req_body[req->request_body_size] = '\0';

    cJSON *item_json = NULL;
    cJSON_ArrayForEach(item_json, dht) {
      char *item_node_id =
          cJSON_GetObjectItemCaseSensitive(item_json, "node_id")->valuestring;

      if (strcmp(req_body, item_node_id) == 0) {
        cJSON_ReplaceItemInObjectCaseSensitive(item_json, "pins",
                                               cJSON_CreateArray());

        sonic_server_response_t *resp =
            sonic_new_response(STATUS_200, MIME_TEXT_PLAIN);

        char *body = "OK";
        sonic_response_set_body(resp, body, strlen(body));
        sonic_send_response(req, resp);

        sonic_free_server_response(resp);

        dht_ctx->saved = false;

        return;
      }
    }

    sonic_server_response_t *resp =
        sonic_new_response(STATUS_406, MIME_TEXT_PLAIN);

    char *body = "ERR peer not found";
    sonic_response_set_body(resp, body, strlen(body));
    sonic_send_response(req, resp);

    sonic_free_server_response(resp);

    dht_ctx->saved = false;

    return;
  } else {
    sonic_server_response_t *resp =
        sonic_new_response(STATUS_406, MIME_TEXT_PLAIN);

    char err[256];
    sprintf(err, "ERR %s", res.error_message);

    sonic_response_set_body(resp, err, strlen(err));
    sonic_send_response(req, resp);
    sonic_free_server_response(resp);
  }
}

void dht_get_peers_with_pin_route(sonic_server_request_t *req) {
  result_t res = db_get_value(dht_ctx, "dht");

  if (is_ok(res)) {
    db_value_t *value = VALUE(res);
    cJSON *dht = value->value_json;

    char *req_body = malloc((req->request_body_size + 1) * sizeof(char));
    strncpy(req_body, req->request_body, req->request_body_size);
    req_body[req->request_body_size] = '\0';

    cJSON *peers_with_pin = cJSON_CreateArray();

    cJSON *item_json = NULL;
    cJSON_ArrayForEach(item_json, dht) {
      cJSON *pins = cJSON_GetObjectItemCaseSensitive(item_json, "pins");

      cJSON *pin_json = NULL;
      cJSON_ArrayForEach(pin_json, pins) {
        char *pin_shoggoth_id = pin_json->valuestring;

        if (strcmp(pin_shoggoth_id, req_body) == 0) {
          cJSON_AddItemReferenceToArray(peers_with_pin, item_json);
          break;
        }
      }
    }

    sonic_server_response_t *resp =
        sonic_new_response(STATUS_406, MIME_TEXT_PLAIN);

    char *body = cJSON_Print(peers_with_pin);
    sonic_response_set_body(resp, body, strlen(body));
    sonic_send_response(req, resp);

    free(body);
    cJSON_Delete(peers_with_pin);
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
  }
}

void dht_peer_pins_add_resource_route(sonic_server_request_t *req) {
  char *node_id = sonic_get_path_segment(req, "node_id");

  result_t res = db_get_value(dht_ctx, "dht");

  if (is_ok(res)) {
    db_value_t *value = VALUE(res);
    cJSON *dht = value->value_json;

    char *req_body = malloc((req->request_body_size + 1) * sizeof(char));
    strncpy(req_body, req->request_body, req->request_body_size);
    req_body[req->request_body_size] = '\0';

    char *shoggoth_id = req_body;

    cJSON *item_json = NULL;
    cJSON_ArrayForEach(item_json, dht) {
      char *item_node_id =
          cJSON_GetObjectItemCaseSensitive(item_json, "node_id")->valuestring;

      if (strcmp(node_id, item_node_id) == 0) {
        cJSON *pins = cJSON_GetObjectItemCaseSensitive(item_json, "pins");

        cJSON *pin_json = cJSON_CreateString(shoggoth_id);
        cJSON_AddItemToArray(pins, pin_json);

        sonic_server_response_t *resp =
            sonic_new_response(STATUS_200, MIME_TEXT_PLAIN);

        char *body = "OK";
        sonic_response_set_body(resp, body, strlen(body));
        sonic_send_response(req, resp);

        sonic_free_server_response(resp);

        dht_ctx->saved = false;

        return;
      }
    }

    sonic_server_response_t *resp =
        sonic_new_response(STATUS_406, MIME_TEXT_PLAIN);

    char *body = "ERR peer not found";
    sonic_response_set_body(resp, body, strlen(body));
    sonic_send_response(req, resp);

    sonic_free_server_response(resp);

    dht_ctx->saved = false;

    return;
  } else {
    sonic_server_response_t *resp =
        sonic_new_response(STATUS_406, MIME_TEXT_PLAIN);

    char err[256];
    sprintf(err, "ERR %s", res.error_message);

    sonic_response_set_body(resp, err, strlen(err));
    sonic_send_response(req, resp);
    sonic_free_server_response(resp);
  }
}

void get_dht_route(sonic_server_request_t *req) {
  result_t res = db_get_value(dht_ctx, "dht");

  if (is_ok(res)) {
    db_value_t *value = VALUE(res);

    if (value->value_type == VALUE_JSON) {
      char *str = cJSON_Print(value->value_json);

      char *body = malloc((strlen(str) + 10) * sizeof(char));
      sprintf(body, "%s %s", value_type_to_str(VALUE_JSON), str);

      sonic_server_response_t *resp =
          sonic_new_response(STATUS_200, MIME_TEXT_PLAIN);
      sonic_response_set_body(resp, body, strlen(body));
      sonic_send_response(req, resp);

      free(str);
      free(body);

      sonic_free_server_response(resp);
    } else {
      sonic_server_response_t *resp =
          sonic_new_response(STATUS_406, MIME_TEXT_PLAIN);

      char err[256];
      sprintf(err, "ERR dht is not a JSON type");

      sonic_response_set_body(resp, err, strlen(err));
      sonic_send_response(req, resp);
      sonic_free_server_response(resp);
    }
  } else {
    sonic_server_response_t *resp =
        sonic_new_response(STATUS_406, MIME_TEXT_PLAIN);

    char err[256];
    sprintf(err, "ERR %s", res.error_message);

    sonic_response_set_body(resp, err, strlen(err));
    sonic_send_response(req, resp);
    sonic_free_server_response(resp);
  }
}

void dht_remove_item_route(sonic_server_request_t *req) {
  result_t res = db_get_value(dht_ctx, "dht");

  if (is_ok(res)) {
    db_value_t *value = VALUE(res);
    cJSON *dht = value->value_json;

    char *req_body = malloc((req->request_body_size + 1) * sizeof(char));
    strncpy(req_body, req->request_body, req->request_body_size);
    req_body[req->request_body_size] = '\0';

    char *node_id = req_body;
    int index = 0;

    const cJSON *item_json = NULL;
    cJSON_ArrayForEach(item_json, dht) {
      char *item_node_id =
          cJSON_GetObjectItemCaseSensitive(item_json, "node_id")->valuestring;

      if (strcmp(node_id, item_node_id) == 0) {
        cJSON_DeleteItemFromArray(dht, index);

        sonic_server_response_t *resp =
            sonic_new_response(STATUS_200, MIME_TEXT_PLAIN);

        char *body = "OK";
        sonic_response_set_body(resp, body, strlen(body));
        sonic_send_response(req, resp);

        free(req_body);
        sonic_free_server_response(resp);

        dht_ctx->saved = false;

        return;
      }

      index++;
    }

    sonic_server_response_t *resp =
        sonic_new_response(STATUS_406, MIME_TEXT_PLAIN);

    char err[256];
    sprintf(err, "ERR item with node_id not found");

    sonic_response_set_body(resp, err, strlen(err));
    sonic_send_response(req, resp);
    sonic_free_server_response(resp);
  } else {
    sonic_server_response_t *resp =
        sonic_new_response(STATUS_406, MIME_TEXT_PLAIN);

    char err[256];
    sprintf(err, "ERR %s", res.error_message);

    sonic_response_set_body(resp, err, strlen(err));
    sonic_send_response(req, resp);
    sonic_free_server_response(resp);
  }
}

void dht_get_unreachable_count_route(sonic_server_request_t *req) {
  result_t res = db_get_value(dht_ctx, "dht");

  if (is_ok(res)) {
    db_value_t *value = VALUE(res);
    free_result(res);
    cJSON *dht = value->value_json;

    char *req_body = malloc((req->request_body_size + 1) * sizeof(char));
    strncpy(req_body, req->request_body, req->request_body_size);
    req_body[req->request_body_size] = '\0';

    char *node_id = req_body;

    const cJSON *item_json = NULL;
    cJSON_ArrayForEach(item_json, dht) {
      char *item_node_id =
          cJSON_GetObjectItemCaseSensitive(item_json, "node_id")->valuestring;

      if (strcmp(node_id, item_node_id) == 0) {
        int unreachable_count =
            cJSON_GetObjectItemCaseSensitive(item_json, "unreachable_count")
                ->valueint;

        sonic_server_response_t *resp =
            sonic_new_response(STATUS_200, MIME_TEXT_PLAIN);

        char body[256];
        sprintf(body, "\"%d\"", unreachable_count);

        sonic_response_set_body(resp, body, strlen(body));
        sonic_send_response(req, resp);

        free(req_body);
        sonic_free_server_response(resp);

        dht_ctx->saved = false;

        return;
      }
    }

    sonic_server_response_t *resp =
        sonic_new_response(STATUS_406, MIME_TEXT_PLAIN);

    char err[256];
    sprintf(err, "ERR item with node_id not found");

    sonic_response_set_body(resp, err, strlen(err));
    sonic_send_response(req, resp);
    sonic_free_server_response(resp);
  } else {
    free_result(res);
    sonic_server_response_t *resp =
        sonic_new_response(STATUS_406, MIME_TEXT_PLAIN);

    char err[256];
    sprintf(err, "ERR %s", res.error_message);

    sonic_response_set_body(resp, err, strlen(err));
    sonic_send_response(req, resp);
    sonic_free_server_response(resp);
  }
}

void dht_increment_unreachable_count_route(sonic_server_request_t *req) {
  result_t res = db_get_value(dht_ctx, "dht");

  if (is_ok(res)) {
    db_value_t *value = VALUE(res);
    free_result(res);
    cJSON *dht = value->value_json;

    char *req_body = malloc((req->request_body_size + 1) * sizeof(char));
    strncpy(req_body, req->request_body, req->request_body_size);
    req_body[req->request_body_size] = '\0';

    char *node_id = req_body;

    const cJSON *item_json = NULL;
    cJSON_ArrayForEach(item_json, dht) {
      char *item_node_id =
          cJSON_GetObjectItemCaseSensitive(item_json, "node_id")->valuestring;

      if (strcmp(node_id, item_node_id) == 0) {
        cJSON *unreachable_count =
            cJSON_GetObjectItemCaseSensitive(item_json, "unreachable_count");

        unreachable_count->valueint++;

        sonic_server_response_t *resp =
            sonic_new_response(STATUS_200, MIME_TEXT_PLAIN);

        char *body = "OK";

        sonic_response_set_body(resp, body, strlen(body));
        sonic_send_response(req, resp);

        free(req_body);
        sonic_free_server_response(resp);

        dht_ctx->saved = false;

        return;
      }
    }

    sonic_server_response_t *resp =
        sonic_new_response(STATUS_406, MIME_TEXT_PLAIN);

    char err[256];
    sprintf(err, "ERR item with node_id not found");

    sonic_response_set_body(resp, err, strlen(err));
    sonic_send_response(req, resp);
    sonic_free_server_response(resp);
  } else {
    free_result(res);
    sonic_server_response_t *resp =
        sonic_new_response(STATUS_406, MIME_TEXT_PLAIN);

    char err[256];
    sprintf(err, "ERR %s", res.error_message);

    sonic_response_set_body(resp, err, strlen(err));
    sonic_send_response(req, resp);
    sonic_free_server_response(resp);
  }
}

void dht_reset_unreachable_count_route(sonic_server_request_t *req) {
  result_t res = db_get_value(dht_ctx, "dht");

  if (is_ok(res)) {
    db_value_t *value = VALUE(res);
    cJSON *dht = value->value_json;

    char *req_body = malloc((req->request_body_size + 1) * sizeof(char));
    strncpy(req_body, req->request_body, req->request_body_size);
    req_body[req->request_body_size] = '\0';

    char *node_id = req_body;

    const cJSON *item_json = NULL;
    cJSON_ArrayForEach(item_json, dht) {
      char *item_node_id =
          cJSON_GetObjectItemCaseSensitive(item_json, "node_id")->valuestring;

      if (strcmp(node_id, item_node_id) == 0) {
        cJSON *unreachable_count =
            cJSON_GetObjectItemCaseSensitive(item_json, "unreachable_count");

        unreachable_count->valueint = 0;

        sonic_server_response_t *resp =
            sonic_new_response(STATUS_200, MIME_TEXT_PLAIN);

        char *body = "OK";

        sonic_response_set_body(resp, body, strlen(body));
        sonic_send_response(req, resp);

        free(req_body);
        sonic_free_server_response(resp);

        dht_ctx->saved = false;

        return;
      }
    }

    sonic_server_response_t *resp =
        sonic_new_response(STATUS_406, MIME_TEXT_PLAIN);

    char err[256];
    sprintf(err, "ERR item with node_id not found");

    sonic_response_set_body(resp, err, strlen(err));
    sonic_send_response(req, resp);
    sonic_free_server_response(resp);
  } else {
    sonic_server_response_t *resp =
        sonic_new_response(STATUS_406, MIME_TEXT_PLAIN);

    char err[256];
    sprintf(err, "ERR %s", res.error_message);

    sonic_response_set_body(resp, err, strlen(err));
    sonic_send_response(req, resp);
    sonic_free_server_response(resp);
  }
}

void dht_add_item_route(sonic_server_request_t *req) {
  result_t res = db_get_value(dht_ctx, "dht");

  if (is_ok(res)) {
    db_value_t *value = VALUE(res);
    free_result(res);

    char *req_body = malloc((req->request_body_size + 1) * sizeof(char));
    strncpy(req_body, req->request_body, req->request_body_size);
    req_body[req->request_body_size] = '\0';

    cJSON *item_json = cJSON_Parse(req_body);
    if (item_json == NULL) {
      sonic_server_response_t *resp =
          sonic_new_response(STATUS_406, MIME_TEXT_PLAIN);

      char err[256];
      sprintf(err, "ERR could not parse JSON");

      sonic_response_set_body(resp, err, strlen(err));
      sonic_send_response(req, resp);
      sonic_free_server_response(resp);

      return;
    }

    free(req_body);

    cJSON_AddItemToArray(value->value_json, item_json);

    sonic_server_response_t *resp =
        sonic_new_response(STATUS_200, MIME_TEXT_PLAIN);

    char *body = "OK";
    sonic_response_set_body(resp, body, strlen(body));
    sonic_send_response(req, resp);

    sonic_free_server_response(resp);

    dht_ctx->saved = false;

    return;
  } else {
    free_result(res);

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

void add_dht_routes(sonic_server_t *server) {
  sonic_add_route(server, "/dht/get_dht", METHOD_GET, get_dht_route);
  sonic_add_route(server, "/dht/add_item", METHOD_GET, dht_add_item_route);
  sonic_add_route(server, "/dht/remove_item", METHOD_GET,
                  dht_remove_item_route);
  sonic_add_route(server, "/dht/peer_clear_pins", METHOD_GET,
                  dht_peer_clear_pins_route);
  sonic_add_route(server, "/dht/peer_pins_add_resource/{node_id}", METHOD_GET,
                  dht_peer_pins_add_resource_route);
  sonic_add_route(server, "/dht/get_peers_with_pin", METHOD_GET,
                  dht_get_peers_with_pin_route);
  sonic_add_route(server, "/dht/get_unreachable_count", METHOD_GET,
                  dht_get_unreachable_count_route);
  sonic_add_route(server, "/dht/reset_unreachable_count", METHOD_GET,
                  dht_reset_unreachable_count_route);
  sonic_add_route(server, "/dht/increment_unreachable_count", METHOD_GET,
                  dht_increment_unreachable_count_route);
}

result_t setup_dht(db_ctx_t *ctx) {
  dht_ctx = ctx;

  cJSON *value_json = cJSON_CreateArray();

  db_add_json_value(ctx, "dht", value_json);

  cJSON_Delete(value_json);

  return OK(NULL);
}
