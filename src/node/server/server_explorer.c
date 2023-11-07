/**** server_explorer.c ****
 *
 *  Copyright (c) 2023 Shoggoth Systems
 *
 * Part of the Shoggoth project, under the MIT License.
 * See LICENCE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/
#include "api.h"
#include "server_profile.h"

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

  add_profile_routes(ctx, server);
}
