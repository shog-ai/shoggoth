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
#include "../json/json.h"
#include "../utils/utils.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

studio_ctx_t *studio_ctx = NULL;

void respond_error(sonic_server_request_t *req, char *error_message);

studio_model_t *new_studio_model() {
  studio_model_t *model = malloc(sizeof(studio_model_t));

  model->name = NULL;

  return model;
}

studio_models_t *new_studio_models() {
  studio_models_t *models = malloc(sizeof(studio_models_t));

  models->items = NULL;
  models->items_count = 0;

  return models;
}

studio_active_model_t *new_studio_active_model() {
  studio_active_model_t *model = malloc(sizeof(studio_active_model_t));

  model->name = NULL;
  model->status = "unmounted";

  return model;
}

void studio_models_add_model(studio_models_t *models, studio_model_t *item) {
  if (models->items_count == 0) {
    models->items = malloc(sizeof(studio_model_t *));
  } else {
    models->items = realloc(models->items, (models->items_count + 1) *
                                               sizeof(studio_model_t *));
  }

  models->items[models->items_count] = item;

  models->items_count++;
}

void free_studio_model(studio_model_t *model) {
  free(model->name);

  free(model);
}

void free_studio_models(studio_models_t *models) {
  for (u64 i = 0; i < models->items_count; i++) {
    free_studio_model(models->items[i]);
  }

  free(models);
}

result_t update_models() {
  free_studio_models(studio_ctx->state->models);
  studio_ctx->state->models = new_studio_models();

  char *models_path =
      string_from(studio_ctx->runtime_path, "/studio/models", NULL);

  result_t res_models_list = get_files_and_dirs_list(models_path);
  free(models_path);

  files_list_t *models_list = PROPAGATE(res_models_list);

  for (u64 i = 0; i < models_list->files_count; i++) {
    char *name = string_from(models_list->files[i], NULL);

    studio_model_t *model = new_studio_model();
    model->name = name;

    studio_models_add_model(studio_ctx->state->models, model);
  }

  free_files_list(models_list);

  return OK(NULL);
}

result_t update_state() {
  result_t res = update_models();

  PROPAGATE(res);

  return OK(NULL);
}

void api_get_studio_state_route(sonic_server_request_t *req) {
  UNWRAP(update_state());

  result_t res_state_json = json_studio_state_to_json(*studio_ctx->state);
  json_t *state_json = UNWRAP(res_state_json);

  char *state_str = json_to_string(state_json);

  sonic_server_response_t *resp =
      sonic_new_response(STATUS_200, MIME_APPLICATION_JSON);
  sonic_response_set_body(resp, state_str, strlen(state_str));

  sonic_send_response(req, resp);

  sonic_free_server_response(resp);

  free(state_str);
}

void api_index_route(sonic_server_request_t *req) {
  char *body = "Hello world";

  sonic_server_response_t *resp =
      sonic_new_response(STATUS_200, MIME_TEXT_PLAIN);
  sonic_response_set_body(resp, body, strlen(body));

  sonic_send_response(req, resp);

  sonic_free_server_response(resp);
}

void launch_model_server(char *model_name) {
  char model_server_logs_path[FILE_PATH_SIZE];
  sprintf(model_server_logs_path, "%s/studio/model_server_logs.txt",
          studio_ctx->runtime_path);

  pid_t pid = fork();

  studio_ctx->model_server_pid = pid;

  // Create a pipe to send mesages to the db process STDIN
  int studio_to_server_pipe[2];
  if (pipe(studio_to_server_pipe) == -1) {
    PANIC("could not create studio_to_server pipe");
  }

  if (pid == -1) {
    PANIC("could not fork model server process \n");
  } else if (pid == 0) {
    // CHILD PROCESS

    chdir(studio_ctx->runtime_path);

    int logs_fd =
        open(model_server_logs_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (logs_fd == -1) {
      PANIC("could not open model server log file");
    }

    // Redirect stdin to the read end of the pipe
    dup2(studio_to_server_pipe[0], STDIN_FILENO);

    // Redirect stdout and stderr to the logs file
    dup2(logs_fd, STDOUT_FILENO);
    dup2(logs_fd, STDERR_FILENO);

    char server_pid_path[FILE_PATH_SIZE];
    sprintf(server_pid_path, "%s/studio/model_server_pid.txt",
            studio_ctx->runtime_path);

    pid_t server_pid = getpid();
    char server_pid_str[120];
    sprintf(server_pid_str, "%d", server_pid);
    write_to_file(server_pid_path, server_pid_str, strlen(server_pid_str));

    // Execute the executable

    char model_server_executable[FILE_PATH_SIZE];
    sprintf(model_server_executable, "%s/bin/shog_model_server",
            studio_ctx->runtime_path);

    char model_path[FILE_PATH_SIZE];
    sprintf(model_path, "%s/studio/models/%s", studio_ctx->runtime_path,
            model_name);

    execlp(model_server_executable, model_server_executable, "--port", "6967",
           "-m", model_path, "-c", "2048", NULL);

    close(logs_fd);

    char *logs = UNWRAP(read_file_to_string(model_server_logs_path));
    PANIC("error occured while launching model server executable. LOGS:\n%s",
          logs);
  } else {
    // PARENT PROCESS

    sleep(1);

    int status;
    int child_status = waitpid(studio_ctx->model_server_pid, &status, WNOHANG);

    if (child_status == -1) {
      PANIC("waitpid failed");
    } else if (child_status == 0) {
    } else {
      char *logs = UNWRAP(read_file_to_string(model_server_logs_path));
      PANIC("error occured while launching model server executable.\nMODEL "
            "SERVER LOGS:\n%s",
            logs);
    }
  }
}

void api_mount_model_route(sonic_server_request_t *req) {
  char *model_name = sonic_get_path_segment(req, "model_name");

  studio_ctx->state->active_model->status = "mounting";
  studio_ctx->state->active_model->name = strdup(model_name);

  // sleep(5);
  launch_model_server(model_name);

  char *body = string_from("mounted ", model_name, NULL);

  sonic_server_response_t *resp =
      sonic_new_response(STATUS_200, MIME_TEXT_PLAIN);
  sonic_response_set_body(resp, body, strlen(body));

  sonic_send_response(req, resp);

  sonic_free_server_response(resp);

  free(body);

  studio_ctx->state->active_model->status = "mounted";
}

void add_studio_api_routes(sonic_server_t *server) {
  sonic_add_route(server, "/api/", METHOD_GET, api_index_route);

  sonic_add_route(server, "/api/get_state", METHOD_GET,
                  api_get_studio_state_route);

  sonic_add_route(server, "/api/mount_model/{model_name}", METHOD_GET,
                  api_mount_model_route);
}

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
  add_studio_api_routes(server);

  int failed = sonic_start_server(server);
  if (failed) {
    LOG(ERROR, "start studio server failed");
  }

  LOG(INFO, "studio server exited");

  return NULL;
}

studio_state_t *new_studio_state() {
  studio_state_t *state = malloc(sizeof(studio_state_t));

  state->models = new_studio_models();

  state->active_model = new_studio_active_model();

  return state;
}

studio_ctx_t *new_studio_ctx() {
  studio_ctx_t *ctx = malloc(sizeof(studio_ctx_t));

  ctx->runtime_path = NULL;
  ctx->state = new_studio_state();

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
