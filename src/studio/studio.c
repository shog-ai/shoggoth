/**** studio.c ****
 *
 *  Copyright (c) 2023 ShogAI
 *
 * Part of the Shoggoth project, under the MIT License.
 * See LICENSE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#include "studio.h"
#include "../args/args.h"
#include "../include/sonic.h"
#include "../utils/utils.h"

#include <stdlib.h>

studio_ctx_t *studio_ctx = NULL;

void respond_error(sonic_server_request_t *req, char *error_message);

void index_route(sonic_server_request_t *req) {
  char file_path[FILE_PATH_SIZE];
  sprintf(file_path, "%s/studio/html/studio.html", studio_ctx->runtime_path);

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

void add_frontend_routes(sonic_server_t *server) {
  sonic_add_route(server, "/", METHOD_GET, index_route);

  char static_dir[FILE_PATH_SIZE];
  sprintf(static_dir, "%s/studio/static", studio_ctx->runtime_path);

  sonic_add_directory_route(server, "/static", static_dir);
}

/****
 * starts a http server exposing the Shog Studio frontend
 *
 ****/
void *start_studio_server() {
  sonic_server_t *server = sonic_new_server("127.0.0.1", 6968);

  add_frontend_routes(server);

  int failed = sonic_start_server(server);
  if (failed) {
    LOG(ERROR, "start studio server failed");
  }

  LOG(INFO, "studio server exited");

  return NULL;
}

studio_ctx_t *new_studio_ctx() {
  studio_ctx_t *ctx = malloc(sizeof(studio_ctx_t));

  ctx->runtime_path = NULL;

  return ctx;
}

result_t shog_init_studio(args_t *args, bool print_info) {
  studio_ctx_t *ctx = new_studio_ctx();

  if (args->set_config) {
    LOG(INFO, "Initializing studio with custom config file: %s",
        args->config_path);
  } else {
    LOG(INFO, "Initializing studio");
  }

  char default_runtime_path[FILE_PATH_SIZE];
  utils_get_default_runtime_path(default_runtime_path);

  if (args->set_runtime_path) {
    ctx->runtime_path = malloc((strlen(args->runtime_path) + 1) * sizeof(char));
    strcpy(ctx->runtime_path, args->runtime_path);

    LOG(INFO, "Using custom runtime path: %s", ctx->runtime_path);

    if (!dir_exists(ctx->runtime_path)) {
      return ERR("custom runtime path does not exist");
    }
  } else {
    ctx->runtime_path =
        malloc((strlen(default_runtime_path) + 1) * sizeof(char));
    strcpy(ctx->runtime_path, default_runtime_path);

    LOG(INFO, "Using default runtime path: %s", ctx->runtime_path);
  }

  utils_verify_studio_runtime_dirs(ctx);

  // result_t res = init_node_runtime(ctx, args);
  // PROPAGATE(res);

  if (print_info) {
    // LOG(INFO, "STUDIO HOST: %s", ctx->config->network.host);
    // LOG(INFO, "STUDIO PORT: %d", ctx->config->network.port);
    // LOG(INFO, "WEB INTERFACE: %s", ctx->config->network.public_host);

    // LOG(INFO, "DB HOST: %s", ctx->config->db.host);
    // LOG(INFO, "DB PORT: %d", ctx->config->db.port);
  }

  // printf("\n\n");

  return OK(ctx);
}

result_t start_studio(args_t *args) {
  result_t res_ctx = shog_init_studio(args, true);
  studio_ctx_t *ctx = PROPAGATE(res_ctx);

  studio_ctx = ctx;

  LOG(INFO, "Starting Shog Studio at http://127.0.0.1:6968");

  start_studio_server();

  return OK(NULL);
}
