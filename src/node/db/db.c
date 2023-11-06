/**** db.c ****
 *
 *  Copyright (c) 2023 Shoggoth Systems
 *
 * Part of the Shoggoth project, under the MIT License.
 * See LICENCE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#include <netlibc.h>

#include "../../include/redis.h"
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

result_t kill_redis_db(node_ctx_t *ctx) {
  char node_runtime_path[FILE_PATH_SIZE];
  utils_get_node_runtime_path(ctx, node_runtime_path);

  char db_pid_path[FILE_PATH_SIZE];
  sprintf(db_pid_path, "%s/db_pid.txt", node_runtime_path);

  if (utils_file_exists(db_pid_path)) {
    result_t res_db_pid_str = utils_read_file_to_string(db_pid_path);
    char *db_pid_str = PROPAGATE(res_db_pid_str);

    int pid = atoi(db_pid_str);
    free(db_pid_str);

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

    utils_delete_file(db_pid_path);
  }

  return OK(NULL);
}

/****
 * launches a redis database child process
 *
 ****/
void launch_db(node_ctx_t *ctx) {
  result_t res_db = kill_redis_db(ctx);
  UNWRAP(res_db);

  char node_runtime_path[FILE_PATH_SIZE];
  utils_get_node_runtime_path(ctx, node_runtime_path);

  char db_logs_path[FILE_PATH_SIZE];
  sprintf(db_logs_path, "%s/redis_db_logs.txt", node_runtime_path);

  pid_t pid = fork();

  ctx->redis_db_pid = pid;

  // Create a pipe to send mesages to the db process STDIN
  int node_to_db_pipe[2];
  if (pipe(node_to_db_pipe) == -1) {
    PANIC("could not create node_to_db pipe");
  }

  if (pid == -1) {
    PANIC("could not fork redis db process \n");
  } else if (pid == 0) {
    // CHILD PROCESS

    int logs_fd = open(db_logs_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (logs_fd == -1) {
      PANIC("could not open redis log file");
    }

    // Redirect stdin to the read end of the pipe
    dup2(node_to_db_pipe[0], STDIN_FILENO);

    // Redirect stdout and stderr to the logs file
    dup2(logs_fd, STDOUT_FILENO);
    dup2(logs_fd, STDERR_FILENO);

    char db_pid_path[FILE_PATH_SIZE];
    sprintf(db_pid_path, "%s/db_pid.txt", node_runtime_path);

    pid_t db_pid = getpid();
    char db_pid_str[120];
    sprintf(db_pid_str, "%d", db_pid);
    utils_write_to_file(db_pid_path, db_pid_str, strlen(db_pid_str));

    // Execute the redis executable

    char redis_executable[FILE_PATH_SIZE];
    sprintf(redis_executable, "%s/bin/redis-server", node_runtime_path);

    char config_path[FILE_PATH_SIZE];
    sprintf(config_path, "%s/redis.conf", node_runtime_path);

    char config_arg[FILE_PATH_SIZE];
    strcpy(config_arg, config_path);

    char port_arg[FILE_PATH_SIZE];
    strcpy(port_arg, "--port ");
    char port_str[16];
    sprintf(port_str, "%d", ctx->config->db.port);
    strcat(port_arg, port_str);

    char dir_arg[FILE_PATH_SIZE];
    strcpy(dir_arg, "--dir ");
    strcat(dir_arg, node_runtime_path);

    char redisjson_arg[FILE_PATH_SIZE];
    strcpy(redisjson_arg, "--loadmodule ");
    strcat(redisjson_arg, node_runtime_path);
    strcat(redisjson_arg, "/bin/redisjson.so");

    execlp(redis_executable, redis_executable, config_arg, port_arg, dir_arg,
           redisjson_arg, NULL);

    close(logs_fd);

    char *db_logs = UNWRAP(utils_read_file_to_string(db_logs_path));
    PANIC("error occured while launching redis executable. LOGS:\n%s", db_logs);
  } else {
    // PARENT PROCESS

    sleep(1);

    int status;
    int child_status = waitpid(ctx->redis_db_pid, &status, WNOHANG);

    if (child_status == -1) {
      PANIC("waitpid failed");
    } else if (child_status == 0) {
    } else {
      char *db_logs = UNWRAP(utils_read_file_to_string(db_logs_path));
      PANIC("error occured while launching redis executable.\nDB LOGS:\n%s",
            db_logs);
    }
  }
}

/****
 * gets the dht from the redis database as an allocated string
 *
 ****/
result_t db_get_dht_str(node_ctx_t *ctx) {
  redis_command_t command = gen_command(COM_GET, "dht", NULL);

  result_t res_str =
      redis_read(ctx->config->db.host, ctx->config->db.port, command);
  char *str = PROPAGATE(res_str);

  return OK(str);
}

/****
 * gets the pins from the redis database as an allocated string
 *
 ****/
result_t db_get_pins_str(node_ctx_t *ctx) {
  redis_command_t command = gen_command(COM_GET, "pins", NULL);

  result_t res =
      redis_read(ctx->config->db.host, ctx->config->db.port, command);
  char *str = PROPAGATE(res);

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

  redis_command_t command = gen_command(COM_APPEND, "dht $", item_str);

  result_t res =
      redis_write(ctx->config->db.host, ctx->config->db.port, command);
  UNWRAP(res);

  free(item_str);

  return OK(NULL);
}

result_t db_dht_remove_item(node_ctx_t *ctx, char *node_id) {
  char filter[512];
  sprintf(filter, "dht '$[?(@.node_id == \"%s\")]'", node_id);

  redis_command_t command = gen_command(COM_DEL, filter, "");

  result_t res =
      redis_write(ctx->config->db.host, ctx->config->db.port, command);
  UNWRAP(res);

  return OK(NULL);
}

result_t db_increment_unreachable_count(node_ctx_t *ctx, char *node_id) {
  char filter[256];
  sprintf(filter, "dht '$[?(@.node_id == \"%s\")].unreachable_count'", node_id);

  redis_command_t command = gen_command(COM_INCREMENT, filter, "1");

  result_t res =
      redis_write(ctx->config->db.host, ctx->config->db.port, command);
  UNWRAP(res);

  return OK(NULL);
}

result_t db_reset_unreachable_count(node_ctx_t *ctx, char *node_id) {
  char filter[256];
  sprintf(filter, "dht '$[?(@.node_id == \"%s\")].unreachable_count'", node_id);

  redis_command_t command = gen_command(COM_SET, filter, "0");

  result_t res =
      redis_write(ctx->config->db.host, ctx->config->db.port, command);
  UNWRAP(res);

  return OK(NULL);
}

result_t db_get_unreachable_count(node_ctx_t *ctx, char *node_id) {
  char filter[256];
  sprintf(filter, "dht '$[?(@.node_id == \"%s\")].unreachable_count'", node_id);

  redis_command_t command = gen_command(COM_GET, filter, NULL);

  result_t res =
      redis_read(ctx->config->db.host, ctx->config->db.port, command);
  char *res_str = UNWRAP(res);

  return OK(res_str);
}

/****
 * adds an item to the database pins
 *
 ****/
result_t db_pins_add_profile(node_ctx_t *ctx, char *shoggoth_id) {

  char data[256];
  sprintf(data, "\"%s\"", shoggoth_id);

  redis_command_t command = gen_command(COM_APPEND, "pins $", data);

  result_t res =
      redis_write(ctx->config->db.host, ctx->config->db.port, command);
  UNWRAP(res);

  return OK(NULL);
}

result_t db_pins_remove_profile(node_ctx_t *ctx, char *shoggoth_id) {

  char filter[512];
  sprintf(filter, "pins '$[?(@ == \"%s\")]'", shoggoth_id);

  redis_command_t command = gen_command(COM_DEL, filter, "");

  result_t res =
      redis_write(ctx->config->db.host, ctx->config->db.port, command);
  UNWRAP(res);

  return OK(NULL);
}

result_t db_get_peers_with_pin(node_ctx_t *ctx, char *shoggoth_id) {
  char filter[512];
  sprintf(filter, "dht '$[?(@.pins[?(@==\"%s\")])]'", shoggoth_id);

  redis_command_t command = gen_command(COM_GET, filter, NULL);

  result_t res =
      redis_read(ctx->config->db.host, ctx->config->db.port, command);
  char *res_str = UNWRAP(res);

  return OK(res_str);
}

result_t db_clear_peer_pins(node_ctx_t *ctx, char *node_id) {
  char filter[512];
  sprintf(filter, "dht '$.[?(@.node_id==\"%s\")].pins'", node_id);

  redis_command_t command = gen_command(COM_SET, filter, "[]");
  result_t res =
      redis_write(ctx->config->db.host, ctx->config->db.port, command);
  UNWRAP(res);

  return OK(NULL);
}

result_t db_peer_pins_add_profile(node_ctx_t *ctx, char *node_id,
                                  char *shoggoth_id) {
  char data[256];
  sprintf(data, "\"%s\"", shoggoth_id);

  char filter[512];
  sprintf(filter, "dht '$.[?(@.node_id==\"%s\")].pins'", node_id);

  redis_command_t command = gen_command(COM_APPEND, filter, data);
  result_t res =
      redis_write(ctx->config->db.host, ctx->config->db.port, command);
  UNWRAP(res);

  return OK(NULL);
}

/****
 * checks if the database contains valid data and initializes it if not
 *
 ****/
result_t db_verify_data(node_ctx_t *ctx) {
  result_t res_local_dht = db_get_dht_str(ctx);
  char *local_dht = PROPAGATE(res_local_dht);

  if (local_dht == NULL) {
    redis_command_t dht_command = gen_command(COM_SET, "dht $", "[]");

    result_t res_dht =
        redis_write(ctx->config->db.host, ctx->config->db.port, dht_command);
    PROPAGATE(res_dht);

    result_t res_gen_peers = generate_default_peers(ctx);
    PROPAGATE(res_gen_peers);
  }

  free(local_dht);

  redis_command_t command = gen_command(COM_SET, "pins $", "[]");

  result_t res =
      redis_write(ctx->config->db.host, ctx->config->db.port, command);
  PROPAGATE(res);

  char pins_path[256];
  utils_get_node_runtime_path(ctx, pins_path);
  strcat(pins_path, "/pins/");

  result_t res_pins_list = utils_get_files_and_dirs_list(pins_path);
  files_list_t *pins_list = PROPAGATE(res_pins_list);

  for (u64 i = 0; i < pins_list->files_count; i++) {
    result_t res_pin_str = utils_remove_file_extension(pins_list->files[i]);
    char *pin_str = PROPAGATE(res_pin_str);

    db_pins_add_profile(ctx, pin_str);

    free(pin_str);
  }

  utils_free_files_list(pins_list);

  return OK(NULL);
}