/**** server_explorer.c ****
 *
 *  Copyright (c) 2023 ShogAI - https://shog.ai
 *
 * Part of the Shoggoth project, under the MIT License.
 * See LICENSE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/
#include "../../../handlebazz/handlebazz.h"
#include "../../json/json.h"
#include "../db/db.h"
#include "api.h"

#include <stdlib.h>

#include <netlibc/mem.h>

node_ctx_t *explorer_ctx = NULL;

void explorer_route(sonic_server_request_t *req) {
  char explorer_dir[FILE_PATH_SIZE];
  utils_get_node_explorer_path(explorer_ctx, explorer_dir);

  char file_path[FILE_PATH_SIZE];
  sprintf(file_path, "%s/explorer.html", explorer_dir);

  result_t res_file_mapping = map_file(file_path);
  SERVER_ERR(res_file_mapping);
  file_mapping_t *file_mapping = VALUE(res_file_mapping);

  sonic_server_response_t *resp =
      sonic_new_response(STATUS_200, MIME_TEXT_HTML);
  sonic_response_set_body(resp, file_mapping->content,
                          (u64)file_mapping->info.st_size);

  sonic_send_response(req, resp);

  sonic_free_server_response(resp);

  unmap_file(file_mapping);
}

void stats_route(sonic_server_request_t *req) {
  char explorer_dir[FILE_PATH_SIZE];
  utils_get_node_explorer_path(explorer_ctx, explorer_dir);

  char file_path[FILE_PATH_SIZE];
  sprintf(file_path, "%s/stats.html", explorer_dir);

  result_t res_file_mapping = map_file(file_path);
  SERVER_ERR(res_file_mapping);
  file_mapping_t *file_mapping = VALUE(res_file_mapping);

  sonic_server_response_t *resp =
      sonic_new_response(STATUS_200, MIME_TEXT_HTML);
  sonic_response_set_body(resp, file_mapping->content,
                          (u64)file_mapping->info.st_size);

  sonic_send_response(req, resp);

  sonic_free_server_response(resp);

  unmap_file(file_mapping);
}

void docs_route(sonic_server_request_t *req) {
  char explorer_dir[FILE_PATH_SIZE];
  utils_get_node_explorer_path(explorer_ctx, explorer_dir);

  char file_path[FILE_PATH_SIZE];
  sprintf(file_path, "%s/docs/docs.html", explorer_dir);

  result_t res_file_mapping = map_file(file_path);
  SERVER_ERR(res_file_mapping);
  file_mapping_t *file_mapping = VALUE(res_file_mapping);

  sonic_server_response_t *resp =
      sonic_new_response(STATUS_200, MIME_TEXT_HTML);
  sonic_response_set_body(resp, file_mapping->content,
                          (u64)file_mapping->info.st_size);

  sonic_send_response(req, resp);

  sonic_free_server_response(resp);

  unmap_file(file_mapping);
}

void docs_api_route(sonic_server_request_t *req) {
  char explorer_dir[FILE_PATH_SIZE];
  utils_get_node_explorer_path(explorer_ctx, explorer_dir);

  char file_path[FILE_PATH_SIZE];
  sprintf(file_path, "%s/docs/api.html", explorer_dir);

  result_t res_file_mapping = map_file(file_path);
  SERVER_ERR(res_file_mapping);
  file_mapping_t *file_mapping = VALUE(res_file_mapping);

  sonic_server_response_t *resp =
      sonic_new_response(STATUS_200, MIME_TEXT_HTML);
  sonic_response_set_body(resp, file_mapping->content,
                          (u64)file_mapping->info.st_size);

  sonic_send_response(req, resp);

  sonic_free_server_response(resp);

  unmap_file(file_mapping);
}

void docs_sonic_route(sonic_server_request_t *req) {
  char explorer_dir[FILE_PATH_SIZE];
  utils_get_node_explorer_path(explorer_ctx, explorer_dir);

  char file_path[FILE_PATH_SIZE];
  sprintf(file_path, "%s/docs/sonic.html", explorer_dir);

  result_t res_file_mapping = map_file(file_path);
  SERVER_ERR(res_file_mapping);
  file_mapping_t *file_mapping = VALUE(res_file_mapping);

  sonic_server_response_t *resp =
      sonic_new_response(STATUS_200, MIME_TEXT_HTML);
  sonic_response_set_body(resp, file_mapping->content,
                          (u64)file_mapping->info.st_size);

  sonic_send_response(req, resp);

  sonic_free_server_response(resp);

  unmap_file(file_mapping);
}

void docs_camel_route(sonic_server_request_t *req) {
  char explorer_dir[FILE_PATH_SIZE];
  utils_get_node_explorer_path(explorer_ctx, explorer_dir);

  char file_path[FILE_PATH_SIZE];
  sprintf(file_path, "%s/docs/camel.html", explorer_dir);

  result_t res_file_mapping = map_file(file_path);
  SERVER_ERR(res_file_mapping);
  file_mapping_t *file_mapping = VALUE(res_file_mapping);

  sonic_server_response_t *resp =
      sonic_new_response(STATUS_200, MIME_TEXT_HTML);
  sonic_response_set_body(resp, file_mapping->content,
                          (u64)file_mapping->info.st_size);

  sonic_send_response(req, resp);

  sonic_free_server_response(resp);

  unmap_file(file_mapping);
}

void docs_tuwi_route(sonic_server_request_t *req) {
  char explorer_dir[FILE_PATH_SIZE];
  utils_get_node_explorer_path(explorer_ctx, explorer_dir);

  char file_path[FILE_PATH_SIZE];
  sprintf(file_path, "%s/docs/tuwi.html", explorer_dir);

  result_t res_file_mapping = map_file(file_path);
  SERVER_ERR(res_file_mapping);
  file_mapping_t *file_mapping = VALUE(res_file_mapping);

  sonic_server_response_t *resp =
      sonic_new_response(STATUS_200, MIME_TEXT_HTML);
  sonic_response_set_body(resp, file_mapping->content,
                          (u64)file_mapping->info.st_size);

  sonic_send_response(req, resp);

  sonic_free_server_response(resp);

  unmap_file(file_mapping);
}

void home_route(sonic_server_request_t *req) {
  sonic_server_response_t *resp =
      sonic_new_response(STATUS_302, MIME_TEXT_PLAIN);

  sonic_response_add_header(resp, "Access-Control-Allow-Origin", "*");
  sonic_response_add_header(resp, "Location", "/explorer");

  sonic_send_response(req, resp);
  sonic_free_server_response(resp);
}

void resource_route(sonic_server_request_t *req) {
  char *shoggoth_id = sonic_get_path_segment(req, "shoggoth_id");

  if (strlen(shoggoth_id) != 68) {
    sonic_server_response_t *resp =
        sonic_new_response(STATUS_404, MIME_TEXT_HTML);

    char *body = "invalid Shoggoth ID";

    sonic_response_set_body(resp, body, (u64)strlen(body));

    sonic_send_response(req, resp);

    sonic_free_server_response(resp);

    return;
  }

  char runtime_path[FILE_PATH_SIZE];
  utils_get_node_runtime_path(explorer_ctx, runtime_path);

  char resource_path[FILE_PATH_SIZE];
  sprintf(resource_path, "%s/pins/%s", runtime_path, shoggoth_id);

  if (!file_exists(resource_path)) {
    result_t res_peers_with_pin =
        db_get_peers_with_pin(explorer_ctx, shoggoth_id);
    char *peers_with_pin = UNWRAP(res_peers_with_pin);

    result_t res_peers = json_string_to_dht(peers_with_pin);
    dht_t *peers = UNWRAP(res_peers);
    nfree(peers_with_pin);

    if (peers->items_count > 0) {
      char location[256];
      sprintf(location, "%s/explorer/resource/%s", peers->items[0]->host,
              shoggoth_id);

      sonic_server_response_t *resp =
          sonic_new_response(STATUS_302, MIME_TEXT_PLAIN);
      sonic_response_add_header(resp, "Location", location);

      sonic_send_response(req, resp);
      sonic_free_server_response(resp);

    } else {
      sonic_server_response_t *resp =
          sonic_new_response(STATUS_404, MIME_TEXT_HTML);

      char *body = "resource not found";

      sonic_response_set_body(resp, body, (u64)strlen(body));

      sonic_send_response(req, resp);

      sonic_free_server_response(resp);
    }

    free_dht(peers);
    return;
  }

  char explorer_dir[FILE_PATH_SIZE];
  utils_get_node_explorer_path(explorer_ctx, explorer_dir);

  char head_template_path[FILE_PATH_SIZE];
  sprintf(head_template_path, "%s/templates/head.html", explorer_dir);

  char end_template_path[FILE_PATH_SIZE];
  sprintf(end_template_path, "%s/templates/end.html", explorer_dir);

  char resource_template_path[FILE_PATH_SIZE];
  sprintf(resource_template_path, "%s/templates/resource.html", explorer_dir);

  result_t res_end_template_string = read_file_to_string(end_template_path);
  char *end_template_string = UNWRAP(res_end_template_string);

  template_t *end_template = create_template(end_template_string, "{}");
  nfree(end_template_string);

  result_t res_head_template_string = read_file_to_string(head_template_path);
  char *head_template_string = UNWRAP(res_head_template_string);

  char *head_template_data = nmalloc(1024 * sizeof(char));

  sprintf(head_template_data,
          "{\"title\": \"Shoggoth Explorer - %s\", \"desc\": \"%s on Shoggoth "
          "- Shoggoth is a peer-to-peer network for publishing and "
          "distributing open-source code, Machine Learning models, datasets, "
          "and research papers.\","
          "\"og_url\": \"%s/explorer/resource_og/%s\", "
          "\"url\": \"%s/explorer/resource/%s\", \"is_resource\": true}",
          shoggoth_id, shoggoth_id, explorer_ctx->config->network.public_host,
          shoggoth_id, explorer_ctx->config->network.public_host, shoggoth_id);

  template_t *head_template =
      create_template(head_template_string, head_template_data);
  nfree(head_template_string);

  result_t res_file_content = read_file_to_string(resource_template_path);
  char *file_content = UNWRAP(res_file_content);

  char *label = UNWRAP(db_get_pin_label(explorer_ctx, shoggoth_id));

  char *download_link =
      string_from(explorer_ctx->config->network.public_host, "/api/download/",
                  shoggoth_id, "/", label, NULL);

  u64 resource_size = UNWRAP_U64(get_dir_size(resource_path));

  char resource_size_str[256];
  if (resource_size < 1024) {
    sprintf(resource_size_str, U64_FORMAT_SPECIFIER " B", resource_size);
  } else if (resource_size < 1024 * 1024) {
    sprintf(resource_size_str, "%.2f KB\n", (float)resource_size / 1024);
  } else if (resource_size < 1024 * 1024 * 1024) {
    sprintf(resource_size_str, "%.2f MB\n",
            (float)resource_size / (1024 * 1024));
  } else {
    sprintf(resource_size_str, "%.2f GB\n",
            (float)resource_size / (1024 * 1024 * 1024));
  }

  char *template_data = nmalloc(1024 * sizeof(char));
  sprintf(template_data,
          "{ \"shoggoth_id\": \"%s\", \"label\": \"%s\", \"size\": \"%s\", "
          "\"hash\": \"%s\", \"download_link\": \"%s\" }",
          shoggoth_id, label, resource_size_str, &shoggoth_id[4],
          download_link);

  // char *template_data = "{ \"shoggoth_id\": \"\" }";
  template_t *template_object = create_template(file_content, template_data);

  result_t res_partial =
      template_add_partial(template_object, "head", head_template);
  UNWRAP(res_partial);

  res_partial = template_add_partial(template_object, "end", end_template);
  UNWRAP(res_partial);

  result_t res_cooked = cook_template(template_object);
  char *cooked = UNWRAP(res_cooked);

  free_template(template_object);
  free_template(head_template);
  free_template(end_template);

  nfree(file_content);
  nfree(head_template_data);

  sonic_server_response_t *resp =
      sonic_new_response(STATUS_200, MIME_TEXT_HTML);
  sonic_response_set_body(resp, cooked, (u64)strlen(cooked));

  sonic_send_response(req, resp);

  nfree(cooked);
  sonic_free_server_response(resp);
}

void add_explorer_routes(node_ctx_t *ctx, sonic_server_t *server) {
  explorer_ctx = ctx;

  // serve static files
  char explorer_dir[FILE_PATH_SIZE];
  utils_get_node_explorer_path(ctx, explorer_dir);

  char static_dir[FILE_PATH_SIZE];
  sprintf(static_dir, "%s/static", explorer_dir);

  sonic_add_directory_route(server, "/explorer/static", static_dir);

  sonic_add_route(server, "/", METHOD_GET, home_route);

  sonic_add_route(server, "/explorer", METHOD_GET, explorer_route);
  sonic_add_route(server, "/explorer/stats", METHOD_GET, stats_route);

  sonic_add_route(server, "/explorer/docs", METHOD_GET, docs_route);
  sonic_add_route(server, "/explorer/docs/api", METHOD_GET, docs_api_route);
  sonic_add_route(server, "/explorer/docs/sonic", METHOD_GET, docs_sonic_route);
  sonic_add_route(server, "/explorer/docs/camel", METHOD_GET, docs_camel_route);
  sonic_add_route(server, "/explorer/docs/tuwi", METHOD_GET, docs_tuwi_route);

  sonic_add_route(server, "/explorer/resource/{shoggoth_id}", METHOD_GET,
                  resource_route);
}
