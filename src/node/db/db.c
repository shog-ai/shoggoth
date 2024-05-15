/**** db.c ****
 *
 *  Copyright (c) 2023 ShogAI - https://shog.ai
 *
 * Part of the Shoggoth project, under the MIT License.
 * See LICENSE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#include <netlibc.h>
#include <netlibc/error.h>
#include <netlibc/fs.h>
#include <netlibc/log.h>
#include <netlibc/mem.h>
#include <netlibc/string.h>

#include "../../include/cjson.h"
#include "../../include/shogdb-client.h"
#include "../../include/sonic.h"
#include "../../json/json.h"
#include "../../utils/utils.h"
#include "../manifest/manifest.h"
#include "../node.h"
#include "db.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

result_t kill_db(node_ctx_t *ctx) {
  char node_runtime_path[256];
  utils_get_node_runtime_path(ctx, node_runtime_path);

  char db_pid_path[256];
  sprintf(db_pid_path, "%s/db_pid.txt", node_runtime_path);

  if (file_exists(db_pid_path)) {
    result_t res_db_pid_str = read_file_to_string(db_pid_path);
    char *db_pid_str = PROPAGATE(res_db_pid_str);

    int pid = atoi(db_pid_str);
    nfree(db_pid_str);

    kill(pid, SIGINT);

    u64 kill_count = 0;

    for (;;) {
      int result = kill(pid, 0);

      if (result == -1 && errno == ESRCH) {
        if (kill_count > 0) {
          LOG(INFO, "db process stopped");
        }

        break;
      }

      LOG(INFO, "Waiting for db process to exit ...");

      sleep(1);

      kill_count++;
    }

    delete_file(db_pid_path);
  }

  return OK(NULL);
}

/****
 * launches a database child process
 *
 ****/
void launch_db(node_ctx_t *ctx) {
  result_t res_db = kill_db(ctx);
  UNWRAP(res_db);

  char node_runtime_path[256];
  utils_get_node_runtime_path(ctx, node_runtime_path);

  char *db_logs_path = string_from(node_runtime_path, "/db_logs.txt", NULL);

  pid_t pid = fork();

  ctx->db_pid = pid;

  // Create a pipe to send mesages to the db process STDIN
  int node_to_db_pipe[2];
  if (pipe(node_to_db_pipe) == -1) {
    PANIC("could not create node_to_db pipe");
  }

  if (pid == -1) {
    PANIC("could not fork db process \n");
  } else if (pid == 0) {
    // CHILD PROCESS

    chdir(node_runtime_path);

    int logs_fd = open(db_logs_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (logs_fd == -1) {
      PANIC("could not open db log file");
    }

    // Redirect stdin to the read end of the pipe
    dup2(node_to_db_pipe[0], STDIN_FILENO);

    // Redirect stdout and stderr to the logs file
    dup2(logs_fd, STDOUT_FILENO);
    dup2(logs_fd, STDERR_FILENO);

    char *db_pid_path = string_from(node_runtime_path, "/db_pid.txt", NULL);

    pid_t db_pid = getpid();
    char db_pid_str[120];
    sprintf(db_pid_str, "%d", db_pid);
    write_to_file(db_pid_path, db_pid_str, strlen(db_pid_str));

    nfree(db_pid_path);

    // Execute the executable

    char db_executable[256];
    sprintf(db_executable, "%s/bin/shogdb", ctx->runtime_path);

    char config_path[256];
    sprintf(config_path, "%s/dbconfig.toml", node_runtime_path);

    char config_arg[256];
    strcpy(config_arg, config_path);

    execlp(db_executable, db_executable, config_arg, NULL);

    close(logs_fd);

    char *db_logs = UNWRAP(read_file_to_string(db_logs_path));
    PANIC("error occured while launching db executable. LOGS:\n%s", db_logs);
  } else {
    // PARENT PROCESS

    sleep(1);

    int status;
    int child_status = waitpid(ctx->db_pid, &status, WNOHANG);

    if (child_status == -1) {
      PANIC("waitpid failed");
    } else if (child_status == 0) {
    } else {
      char *db_logs = UNWRAP(read_file_to_string(db_logs_path));
      PANIC("error occured while launching db executable.\nDB LOGS:\n%s",
            db_logs);
    }

    nfree(db_logs_path);
  }
}

result_t shogdb_get_value(node_ctx_t *ctx, char *endpoint, char *body) {
  char url[256];
  sprintf(url, "http://%s:%d/%s", ctx->config->db.host, ctx->config->db.port,
          endpoint);

  sonic_request_t *req = sonic_new_request(METHOD_GET, url);
  if (body != NULL) {
    sonic_set_body(req, body, strlen(body));
  }

  sonic_response_t *resp = sonic_send_request(req);
  sonic_free_request(req);

  if (resp->failed) {
    return ERR("request failed: %s \n", resp->error);
  } else {
    char *response_body =
        nmalloc((resp->response_body_size + 1) * sizeof(char));
    strncpy(response_body, resp->response_body, resp->response_body_size);
    response_body[resp->response_body_size] = '\0';

    nfree(resp->response_body);
    sonic_free_response(resp);

    if (strncmp(response_body, "JSON", 4) == 0) {
      result_t res = shogdb_parse_message(response_body);
      nfree(response_body);

      db_value_t *value = PROPAGATE(res);

      char *str = cJSON_Print(value->value_json);

      free_db_value(value);

      return OK(str);
    } else {
      return OK(response_body);
    }
  }

  return OK(NULL);
}

/****
 * gets the dht from the database as an allocated string
 *
 ****/
result_t db_get_dht_str(node_ctx_t *ctx) {
  result_t res_str = shogdb_get_value(ctx, "dht/get_dht", NULL);
  char *str = PROPAGATE(res_str);

  // LOG(INFO, "DHT: %s", str);

  return OK(str);
}

/****
 * gets the pins from the database as an allocated string
 *
 ****/
result_t db_get_pins_str(node_ctx_t *ctx) {
  result_t res_str = shogdb_get_value(ctx, "pins/get_pins", NULL);
  char *str = PROPAGATE(res_str);

  // LOG(INFO, "PINS: %s", str);

  return OK(str);
}

result_t db_get_pin_label(node_ctx_t *ctx, char *shoggoth_id) {
  char *path = string_from("pins/get_pin_label/", shoggoth_id, NULL);

  result_t res_str = shogdb_get_value(ctx, path, NULL);
  char *str = PROPAGATE(res_str);
  nfree(path);

  return OK(str);
}

/****
 * uses the bootstrap peers to populate the dht
 *
 ****/
result_t generate_default_peers(node_ctx_t *ctx) {
  for (u64 i = 0; i < ctx->config->peers.bootstrap_peers_count; i++) {
    result_t res = add_new_peer(ctx, ctx->config->peers.bootstrap_peers[i]);

    if (is_err(res)) {
      LOG(ERROR, "Could not add bootstrap peer: %s", res.error_message);
    }

    free_result(res);
  }

  return OK(NULL);
}

/****
 * adds an item to the database dht
 *
 ****/
result_t db_dht_add_item(node_ctx_t *ctx, dht_item_t *item) {
  result_t res_item_str = dht_item_to_str(item);
  char *item_str = PROPAGATE(res_item_str);

  result_t res_str = shogdb_get_value(ctx, "dht/add_item", item_str);
  char *str = PROPAGATE(res_str);
  nfree(str);

  nfree(item_str);

  return OK(NULL);
}

result_t db_dht_remove_item(node_ctx_t *ctx, char *node_id) {
  result_t res_str = shogdb_get_value(ctx, "dht/remove_item", node_id);
  char *str = PROPAGATE(res_str);
  nfree(str);

  return OK(NULL);
}

result_t db_increment_unreachable_count(node_ctx_t *ctx, char *node_id) {
  result_t res_str =
      shogdb_get_value(ctx, "dht/increment_unreachable_count", node_id);
  char *str = PROPAGATE(res_str);
  nfree(str);

  return OK(NULL);
}

result_t db_reset_unreachable_count(node_ctx_t *ctx, char *node_id) {
  result_t res_str = shogdb_get_value(ctx, "dht/reset_unreachable_count", node_id);
  char *str = PROPAGATE(res_str);
  nfree(str);

  return OK(NULL);
}

result_t db_get_unreachable_count(node_ctx_t *ctx, char *node_id) {
  result_t res_str = shogdb_get_value(ctx, "dht/get_unreachable_count", node_id);
  char *str = PROPAGATE(res_str);

  return OK(str);
}

/****
 * adds an item to the database pins
 *
 ****/
result_t db_pins_add_resource(node_ctx_t *ctx, char *shoggoth_id, char *label) {
  char *path = string_from("pins/add_resource/", shoggoth_id, "/", label, NULL);

  result_t res_str = shogdb_get_value(ctx, path, NULL);
  char *str = PROPAGATE(res_str);
  nfree(str);
  nfree(path);

  return OK(NULL);
}

result_t db_pins_remove_resource(node_ctx_t *ctx, char *shoggoth_id) {
  char *path = string_from("pins/remove_resource/", shoggoth_id, NULL);

  result_t res_str = shogdb_get_value(ctx, path, NULL);
  char *str = PROPAGATE(res_str);
  nfree(str);

  return OK(NULL);
}

result_t db_get_peers_with_pin(node_ctx_t *ctx, char *shoggoth_id) {
  result_t res_str = shogdb_get_value(ctx, "dht/get_peers_with_pins", shoggoth_id);
  char *str = PROPAGATE(res_str);

  // LOG(INFO, "PINS: %s", str);

  return OK(str);
}

result_t db_clear_peer_pins(node_ctx_t *ctx, char *node_id) {
  result_t res_str = shogdb_get_value(ctx, "dht/peer_clear_pins", node_id);
  char *str = PROPAGATE(res_str);
  nfree(str);

  return OK(NULL);
}

result_t db_peer_pins_add_resource(node_ctx_t *ctx, char *node_id,
                                   char *shoggoth_id) {

  char endpoint[256];
  sprintf(endpoint, "dht/peer_pins_add_resource/%s", node_id);

  result_t res_str = shogdb_get_value(ctx, endpoint, shoggoth_id);
  char *str = PROPAGATE(res_str);
  nfree(str);

  return OK(NULL);
}

result_t db_clear_local_pins(node_ctx_t *ctx) {
  result_t res_str = shogdb_get_value(ctx, "pins/clear", NULL);
  char *str = PROPAGATE(res_str);
  nfree(str);

  return OK(NULL);
}

/****
 * checks if the database contains valid data and initializes it if not
 *
 ****/
result_t db_verify_data(node_ctx_t *ctx) {
  result_t res_local_dht = db_get_dht_str(ctx);
  char *local_dht = PROPAGATE(res_local_dht);

  if (strcmp(local_dht, "[]") == 0) {
    result_t res_gen_peers = generate_default_peers(ctx);
    PROPAGATE(res_gen_peers);
  }

  nfree(local_dht);

  // result_t res_clear = db_clear_local_pins(ctx);
  // PROPAGATE(res_clear);

  // char pins_path[256];
  // utils_get_node_runtime_path(ctx, pins_path);
  // strcat(pins_path, "/pins/");

  // result_t res_pins_list = get_files_and_dirs_list(pins_path);
  // files_list_t *pins_list = PROPAGATE(res_pins_list);

  // for (u64 i = 0; i < pins_list->files_count; i++) {
  //   db_pins_add_resource(ctx, pins_list->files[i], "NONE");
  // }

  // free_files_list(pins_list);

  return OK(NULL);
}
