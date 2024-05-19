/**** api.c ****
 *
 *  Copyright (c) 2023 ShogAI - https://shog.ai
 *
 * Part of the Shoggoth project, under the MIT License.
 * See LICENSE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#include "../../json/json.h"
#include "../db/db.h"
#include "../manifest/manifest.h"

#include <assert.h>
#include <stdlib.h>

#include <netlibc/mem.h>

node_ctx_t *api_ctx = NULL;

typedef enum {
  PROFILE,
  GROUP,
  RESOURCE,
} request_target_t;

void api_download_route(sonic_server_request_t *req) {
  char *shoggoth_id = sonic_get_path_segment(req, "shoggoth_id");
  char *label = sonic_get_path_segment(req, "label");

  if (strlen(shoggoth_id) != 68) {
    respond_error(req, "invalid Shoggoth ID");

    return;
  }

  char runtime_path[256];
  utils_get_node_runtime_path(api_ctx, runtime_path);

  char *pin_dir_path = string_from(runtime_path, "/pins/", shoggoth_id, NULL);

  if (!file_exists(pin_dir_path)) {
    result_t res_peers_with_pin = db_get_peers_with_pin(api_ctx, shoggoth_id);
    char *peers_with_pin = SERVER_PROPAGATE(res_peers_with_pin);

    result_t res_peers = json_string_to_dht(peers_with_pin);
    dht_t *peers = SERVER_PROPAGATE(res_peers);

    nfree(peers_with_pin);

    if (peers->items_count > 0) {
      char location[256];
      sprintf(location, "%s/api/download/%s", peers->items[0]->host,
              shoggoth_id);

      sonic_server_response_t *resp =
          sonic_new_response(STATUS_302, MIME_TEXT_PLAIN);
      sonic_response_add_header(resp, "Location", location);

      sonic_send_response(req, resp);
      sonic_free_server_response(resp);
    } else {
      respond_error(req, "resource not found");
    }

    free_dht(peers);
  } else {
    sonic_server_response_t *resp =
        sonic_new_file_response(STATUS_200, pin_dir_path);

    char *disposition;
    if (label != NULL) {
      disposition = string_from("attachment;filename=\"", label, "\"", NULL);
      sonic_response_add_header(resp, "Content-Disposition", disposition);
    }

    result_t res = sonic_send_response(req, resp);
    free_result(res);

    if (label != NULL) {
      nfree(disposition);
    }

    sonic_free_server_response(resp);
  }

  nfree(pin_dir_path);
}

void add_peer(char *peer_manifest) {
  result_t res_manifest = json_string_to_node_manifest(peer_manifest);
  if (is_err(res_manifest)) {
    return;
  }
  node_manifest_t *manifest = VALUE(res_manifest);

  add_new_peer(api_ctx, manifest->public_host);

  free_node_manifest(manifest);
}

void api_get_dht_route(sonic_server_request_t *req) {
  if (req->request_body_size > 0) {
    char *req_body_str = nmalloc((req->request_body_size + 1) * sizeof(char));
    strncpy(req_body_str, req->request_body, req->request_body_size);

    req_body_str[req->request_body_size] = '\0';

    add_peer(req_body_str);
    nfree(req_body_str);
  }

  result_t res_body = db_get_dht_str(api_ctx);
  char *body = SERVER_PROPAGATE(res_body);

  sonic_server_response_t *resp =
      sonic_new_response(STATUS_200, MIME_APPLICATION_JSON);
  sonic_response_set_body(resp, body, strlen(body));

  sonic_response_add_header(resp, "Access-Control-Allow-Origin", "*");

  sonic_send_response(req, resp);
  sonic_free_server_response(resp);

  nfree(body);
}

void api_get_pins_route(sonic_server_request_t *req) {
  result_t res_pins_str = db_get_pins_str(api_ctx);
  char *pins_str = SERVER_PROPAGATE(res_pins_str);

  char *body = pins_str;

  sonic_server_response_t *resp =
      sonic_new_response(STATUS_200, MIME_APPLICATION_JSON);
  sonic_response_set_body(resp, body, strlen(body));

  sonic_response_add_header(resp, "Access-Control-Allow-Origin", "*");

  sonic_send_response(req, resp);
  nfree(body);
  sonic_free_server_response(resp);
}

void api_get_manifest_route(sonic_server_request_t *req) {
  if (req->request_body_size > 0) {
    char *req_body_str = nmalloc((req->request_body_size + 1) * sizeof(char));
    strncpy(req_body_str, req->request_body, req->request_body_size);

    req_body_str[req->request_body_size] = '\0';

    add_peer(req_body_str);
    nfree(req_body_str);
  }

  result_t res_manifest_json = json_node_manifest_to_json(*api_ctx->manifest);
  json_t *manifest_json = SERVER_PROPAGATE(res_manifest_json);

  char *body = json_to_string(manifest_json);
  free_json(manifest_json);

  sonic_server_response_t *resp =
      sonic_new_response(STATUS_200, MIME_APPLICATION_JSON);

  sonic_response_set_body(resp, body, strlen(body));

  sonic_response_add_header(resp, "Access-Control-Allow-Origin", "*");

  sonic_send_response(req, resp);
  sonic_free_server_response(resp);

  nfree(body);
}

void add_api_routes(node_ctx_t *ctx, sonic_server_t *server) {
  api_ctx = ctx;

  sonic_add_route(server, "/api/download/{shoggoth_id}", METHOD_GET,
                  api_download_route);
  sonic_add_route(server, "/api/download/{shoggoth_id}/{label}", METHOD_GET,
                  api_download_route);

  sonic_add_route(server, "/api/get_dht", METHOD_GET, api_get_dht_route);
  sonic_add_route(server, "/api/get_pins", METHOD_GET, api_get_pins_route);
  sonic_add_route(server, "/api/get_manifest", METHOD_GET,
                  api_get_manifest_route);
}
