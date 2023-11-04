/**** node.c ****
 *
 *  Copyright (c) 2023 Shoggoth Systems
 *
 * Part of the Shoggoth project, under the MIT License.
 * See LICENCE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#include "../const.h"
#include "../json/json.h"
#include "../openssl/openssl.h"
#include "../toml/toml.h"
#include "db/db.h"
#include "garbage_collector/garbage_collector.h"
#include "manifest/manifest.h"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>

node_ctx_t *node_global_ctx;

#ifndef VERSION
#define VERSION "0.0.0"
#endif

/****
 * uses openssl to generate an RSA key-pair
 *
 ****/
result_t generate_node_key_pair(char *keys_path, char *public_key_path,
                                char *private_key_path) {
  printf("%s", NODE_KEYS_WARNING);

  getchar();

  result_t res = openssl_generate_key_pair(private_key_path, public_key_path);
  PROPAGATE(res);

  LOG(INFO, "Key pair generated successfully in %s.\n", keys_path);

  return OK(NULL);
}

/****
 * Initializes the system environment and sets up the node runtime
 *
 ****/
result_t init_node_runtime(node_ctx_t *ctx, args_t *args) {
  chdir(ctx->runtime_path);

  char node_runtime_path[FILE_PATH_SIZE];
  utils_get_node_runtime_path(ctx, node_runtime_path);

  char keys_path[FILE_PATH_SIZE];
  sprintf(keys_path, "%s/keys", node_runtime_path);

  char private_key_path[FILE_PATH_SIZE];
  sprintf(private_key_path, "%s/private.txt", keys_path);

  char public_key_path[FILE_PATH_SIZE];
  sprintf(public_key_path, "%s/public.txt", keys_path);

  if (!utils_dir_exists(keys_path)) {
    utils_create_dir(keys_path);
  }

  if (!utils_keys_exist(keys_path)) {
    result_t res =
        generate_node_key_pair(keys_path, public_key_path, private_key_path);
    PROPAGATE(res);
  }

  char pins_path[FILE_PATH_SIZE];
  sprintf(pins_path, "%s/pins", node_runtime_path);

  if (!utils_dir_exists(pins_path)) {
    utils_create_dir(pins_path);
  }

  char config_path[FILE_PATH_SIZE];

  if (args->set_config) {
    strcpy(config_path, args->config_path);
  } else {
    sprintf(config_path, "%s/config.toml", node_runtime_path);
  }

  if (!utils_file_exists(config_path)) {
    return ERR("Config file `%s` does not exist", config_path);
  }

  result_t res_config_str = utils_read_file_to_string(config_path);
  char *config_str = PROPAGATE(res_config_str);

  result_t res_config = toml_string_to_node_config(config_str);
  node_config_t *config = PROPAGATE(res_config);

  free(config_str);

  ctx->config = config;

  result_t res_public_key_string = utils_read_file_to_string(public_key_path);
  char *public_key_string = PROPAGATE(res_public_key_string);

  result_t res_manifest_str =
      generate_node_manifest(public_key_string, config->network.public_host,
                             config->explorer.enable, VERSION);
  char *manifest_str = PROPAGATE(res_manifest_str);

  free(public_key_string);

  result_t res_manifest = json_string_to_node_manifest(manifest_str);
  node_manifest_t *manifest = PROPAGATE(res_manifest);

  free(manifest_str);

  ctx->manifest = manifest;

  ctx->node_pid = getpid();

  return OK(NULL);
}

/****
 * constructs a new node_ctx_t
 *
 ****/
node_ctx_t *new_node_ctx() {
  node_ctx_t *ctx = malloc(sizeof(node_ctx_t));

  ctx->runtime_path = NULL;
  ctx->db_launched = true;
  ctx->node_server_started = false;
  ctx->should_exit = false;
  ctx->node_http_server = NULL;

  return ctx;
}

/****
 * frees all allocated memory and resources, then exits the process with
 * exit_code
 *
 ****/
void shoggoth_node_exit(int exit_code) {
  if (node_global_ctx->node_server_started) {
    sonic_stop_server(node_global_ctx->node_http_server);
  }

  node_global_ctx->should_exit = true;

  sleep(5);

  if (node_global_ctx->node_server_started) {
    free(node_global_ctx->node_http_server);
  }

  // Send a kill signal to the redis db process
  if (kill(node_global_ctx->redis_db_pid, SIGTERM) == -1) {
    PANIC("kill redis process failed");
  }

  // Wait for the child process to exit
  int status;
  pid_t exited_pid = waitpid(node_global_ctx->redis_db_pid, &status, 0);

  if (exited_pid == -1) {
    PANIC("waitpid failed");
  }

  if (WIFEXITED(status)) {
  } else {
    LOG(ERROR, "redis db child process did not exit normally");
  }

  free(node_global_ctx->runtime_path);

  free(node_global_ctx->config->network.host);
  free(node_global_ctx->config->network.public_host);
  free(node_global_ctx->config->update.id);
  free(node_global_ctx->config->db.host);

  for (u64 i = 0; i < node_global_ctx->config->peers.bootstrap_peers_count;
       i++) {
    free(node_global_ctx->config->peers.bootstrap_peers[i]);
  }
  if (node_global_ctx->config->peers.bootstrap_peers_count > 0) {
    free(node_global_ctx->config->peers.bootstrap_peers);
  }

  free(node_global_ctx->config);

  free(node_global_ctx->manifest->public_host);
  free(node_global_ctx->manifest->public_key);
  free(node_global_ctx->manifest->node_id);
  free(node_global_ctx->manifest->version);

  free(node_global_ctx);

  exit(exit_code);
}

/****
 * handler function for node exit signals
 *
 ****/
void exit_handler(int sig) {
  if (sig == SIGINT || sig == SIGTERM) {
    LOG(INFO, "STOPPING NODE......");
  } else if (sig == SIGSEGV) {
    printf("\nSEGMENTATION FAULT");
    fflush(stdout);
  }

  if (sig == SIGINT || sig == SIGTERM) {
    shoggoth_node_exit(0);
  } else if (sig == SIGSEGV) {
    shoggoth_node_exit(1);
  }
}

/****
 * sets custom handlers for signals SIGINT and SIGSEGV
 *
 ****/
void set_node_signal_handlers() {
  // Set the SIGINT (CTRL-C) handler to the custom handler
  if (signal(SIGINT, exit_handler) == SIG_ERR) {
    PANIC("could not set SIGINT signal handler");
  }

  // Register the signal handler for SIGTERM
  if (signal(SIGTERM, exit_handler) == SIG_ERR) {
    PANIC("could not set SIGTERM signal handler");
  }
}

result_t run_node_server(node_ctx_t *ctx) {
  LOG(INFO, "Starting node server");

  LOG(INFO, "Explorer: http://%s:%d/", ctx->config->network.host,
      ctx->config->network.port);

  set_node_signal_handlers();

  launch_db(ctx);

  db_verify_data(ctx);

  pthread_t server_thread;

  server_thread_args_t server_args = {.ctx = ctx};
  if (pthread_create(&server_thread, NULL, start_node_server, &server_args) !=
      0) {
    PANIC("could not spawn server thread");
  }

  pthread_t dht_thread;
  pthread_t garbage_collector_thread;
  pthread_t pins_downloader_thread;
  pthread_t pins_updater_thread;

  dht_thread_args_t dht_thread_args = {.ctx = ctx};
  if (pthread_create(&dht_thread, NULL, dht_updater, &dht_thread_args) != 0) {
    PANIC("could not spawn dht thread");
  }

  pins_downloader_thread_args_t pins_downloader_thread_args = {.ctx = ctx};
  if (pthread_create(&pins_downloader_thread, NULL, pins_downloader,
                     &pins_downloader_thread_args) != 0) {
    PANIC("could not spawn pins downloader thread");
  }

  pins_updater_thread_args_t pins_updater_thread_args = {.ctx = ctx};
  if (pthread_create(&pins_updater_thread, NULL, pins_updater,
                     &pins_updater_thread_args) != 0) {
    PANIC("could not spawn pins updater thread");
  }

  garbage_collector_thread_args_t garbage_collector_thread_args = {.ctx = ctx};
  if (pthread_create(&garbage_collector_thread, NULL, garbage_collector,
                     &garbage_collector_thread_args) != 0) {
    PANIC("could not spawn garbage collector thread");
  }

  pthread_join(server_thread, NULL);
  pthread_join(dht_thread, NULL);
  pthread_join(garbage_collector_thread, NULL);
  pthread_join(pins_downloader_thread, NULL);
  pthread_join(pins_updater_thread, NULL);

  return OK(NULL);
}

/****
 * starts a shoggoth node
 *
 ****/
result_t shog_init_node(args_t *args, bool print_info) {
  node_ctx_t *ctx = new_node_ctx();

  node_global_ctx = ctx;

  if (args->set_config) {
    LOG(INFO, "Initializing node with custom config file: %s",
        args->config_path);
  } else {
    LOG(INFO, "Initializing node");
  }

  char default_runtime_path[FILE_PATH_SIZE];
  utils_get_default_runtime_path(default_runtime_path);

  // NOTE: custom runtime path must be an absolute path else certain dynamic
  // libraries will not load
  if (args->set_runtime_path) {
    ctx->runtime_path = malloc((strlen(args->runtime_path) + 1) * sizeof(char));
    strcpy(ctx->runtime_path, args->runtime_path);

    LOG(INFO, "Using custom runtime path: %s", ctx->runtime_path);

    if (!utils_dir_exists(ctx->runtime_path)) {
      return ERR("custom runtime path does not exist");
    }
  } else {
    ctx->runtime_path =
        malloc((strlen(default_runtime_path) + 1) * sizeof(char));
    strcpy(ctx->runtime_path, default_runtime_path);

    LOG(INFO, "Using default runtime path: %s", ctx->runtime_path);
  }

  utils_verify_node_runtime_dirs(ctx);

  result_t res = init_node_runtime(ctx, args);
  PROPAGATE(res);

  if (print_info) {
    LOG(INFO, "NODE VERSION: %s", ctx->manifest->version);
    LOG(INFO, "NODE ID: %s", ctx->manifest->node_id);
    LOG(INFO, "NODE HOST: %s", ctx->config->network.host);
    LOG(INFO, "NODE PORT: %d", ctx->config->network.port);
    LOG(INFO, "NODE PUBLIC HOST: %s", ctx->config->network.public_host);

    LOG(INFO, "DB HOST: %s", ctx->config->db.host);
    LOG(INFO, "DB PORT: %d", ctx->config->db.port);
  }

  setbuf(stdout, NULL); // Disable buffering for stdout

  printf("\n\n");

  return OK(ctx);
}

result_t node_print_id(node_ctx_t *ctx) {
  printf("Your Node ID is: %s\n", ctx->manifest->node_id);

  return OK(NULL);
}

result_t start_node_service(node_ctx_t *ctx, args_t *args) {
  LOG(INFO, "Starting node as a service");

  pid_t pid = fork();

  // Create a pipe to send mesages to the child process STDIN
  int parent_to_child_pipe[2];
  if (pipe(parent_to_child_pipe) == -1) {
    PANIC("could not create parent_to_child pipe");
  }

  if (pid == -1) {
    PANIC("could not fork node service process \n");
  } else if (pid == 0) {
    // CHILD PROCESS

    char runtime_path[FILE_PATH_SIZE];
    strcpy(runtime_path, ctx->runtime_path);

    char node_runtime_path[FILE_PATH_SIZE];
    utils_get_node_runtime_path(ctx, node_runtime_path);

    char node_config_path[FILE_PATH_SIZE];

    if (args->set_config) {
      sprintf(node_config_path, "%s", args->config_path);
    } else {
      sprintf(node_config_path, "%s/node/config.toml", runtime_path);
    }

    char node_logs_path[FILE_PATH_SIZE];
    sprintf(node_logs_path, "%s/node_service_logs.txt", node_runtime_path);

    char node_pid_path[FILE_PATH_SIZE];
    sprintf(node_pid_path, "%s/node_service_pid.txt", node_runtime_path);

    pid_t node_pid = getpid();
    char node_pid_str[120];
    sprintf(node_pid_str, "%d", node_pid);
    utils_write_to_file(node_pid_path, node_pid_str, strlen(node_pid_str));

    int logs_fd = open(node_logs_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (logs_fd == -1) {
      PANIC("could not open node log file");
    }

    // Redirect stdin to the read end of the pipe
    dup2(parent_to_child_pipe[0], STDIN_FILENO);

    // Redirect stdout and stderr to the logs file
    dup2(logs_fd, STDOUT_FILENO);
    dup2(logs_fd, STDERR_FILENO);

    // Execute the node executable

    char node_executable[FILE_PATH_SIZE];
    sprintf(node_executable, "%s/bin/shog", runtime_path);

    char config_arg[FILE_PATH_SIZE];
    strcpy(config_arg, node_config_path);

    char node_runtime_arg[FILE_PATH_SIZE];
    strcpy(node_runtime_arg, runtime_path);

    execlp("nohup", "nohup", node_executable, "node", "run", "-c", config_arg,
           "-r", node_runtime_arg, NULL);

    close(logs_fd);
    PANIC("error occured while launching node executable \n");
  } else {
    // PARENT PROCESS
  }

  return OK(NULL);
}

result_t check_node_service(node_ctx_t *ctx) {
  char node_runtime_path[FILE_PATH_SIZE];
  utils_get_node_runtime_path(ctx, node_runtime_path);

  char node_pid_path[FILE_PATH_SIZE];
  sprintf(node_pid_path, "%s/node_service_pid.txt", node_runtime_path);

  result_t res_node_pid_str = utils_read_file_to_string(node_pid_path);
  char *node_pid_str = PROPAGATE(res_node_pid_str);

  if (node_pid_str == NULL) {
    printf("Node service is not running\n");
    return ERR("Node service is not running (pid file not readable)");
  }

  int pid = atoi(node_pid_str);
  free(node_pid_str);

  int result = kill(pid, 0);

  if (result == 0) {
    printf("Node is running\n");
  } else if (result == -1) {
    printf("Node is not running\n");
  }

  return OK(NULL);
}

result_t stop_node_service(node_ctx_t *ctx) {
  char node_runtime_path[FILE_PATH_SIZE];
  utils_get_node_runtime_path(ctx, node_runtime_path);

  char node_pid_path[FILE_PATH_SIZE];
  sprintf(node_pid_path, "%s/node_service_pid.txt", node_runtime_path);

  result_t res_node_pid_str = utils_read_file_to_string(node_pid_path);
  char *node_pid_str = PROPAGATE(res_node_pid_str);

  if (node_pid_str == NULL) {
    return ERR("Node service is not running");
  }

  int pid = atoi(node_pid_str);
  free(node_pid_str);

  kill(pid, SIGINT);

  for (;;) {
    int result = kill(pid, 0);

    if (result == -1 && errno == ESRCH) {
      break;
    }

    LOG(INFO, "Waiting for node process to exit ...");

    sleep(1);
  }

  utils_delete_file(node_pid_path);

  LOG(INFO, "Node service stopped");

  return OK(NULL);
}

result_t print_node_service_logs(node_ctx_t *ctx) {
  char node_runtime_path[FILE_PATH_SIZE];
  utils_get_node_runtime_path(ctx, node_runtime_path);

  char node_logs_path[FILE_PATH_SIZE];
  sprintf(node_logs_path, "%s/node_service_logs.txt", node_runtime_path);

  result_t res_node_logs_str = utils_read_file_to_string(node_logs_path);
  char *node_logs_str = PROPAGATE(res_node_logs_str);

  if (node_logs_str == NULL) {
    return ERR("Node service is not running");
  }

  puts(node_logs_str);

  free(node_logs_str);

  return OK(NULL);
}

result_t restart_node_service(node_ctx_t *ctx, args_t *args) {
  LOG(INFO, "Restarting node service");

  char node_runtime_path[FILE_PATH_SIZE];
  utils_get_node_runtime_path(ctx, node_runtime_path);

  char node_pid_path[FILE_PATH_SIZE];
  sprintf(node_pid_path, "%s/node_service_pid.txt", node_runtime_path);

  result_t res_node_pid_str = utils_read_file_to_string(node_pid_path);
  char *node_pid_str = PROPAGATE(res_node_pid_str);

  if (node_pid_str == NULL) {
    return ERR("Node service is not running");
  }

  int pid = atoi(node_pid_str);
  free(node_pid_str);

  int result = kill(pid, 0);

  if (result == 0) {
    kill(pid, SIGINT);

    for (;;) {
      int result = kill(pid, 0);

      if (result == -1 && errno == ESRCH) {
        break;
      }

      LOG(INFO, "Waiting for node process to exit ...");

      sleep(1);
    }

    utils_delete_file(node_pid_path);

    LOG(INFO, "Node service stopped");

    start_node_service(ctx, args);
  } else if (result == -1) {
    printf("Node is not running\n");
  }

  return OK(NULL);
}

/****
 * handles a node session.
 *
 ****/
result_t handle_node_session(args_t *args) {
  if (args->no_command_arg) {
    return ERR("ARGS: no argument was provided to node command \n");
  } else {
    if (strcmp(args->command_arg, "run") == 0) {
      result_t res_ctx = shog_init_node(args, true);
      node_ctx_t *ctx = PROPAGATE(res_ctx);

      result_t res = run_node_server(ctx);
      PROPAGATE(res);
    } else if (strcmp(args->command_arg, "start") == 0) {
      result_t res_ctx = shog_init_node(args, true);
      node_ctx_t *ctx = PROPAGATE(res_ctx);

      result_t res = start_node_service(ctx, args);
      PROPAGATE(res);
    } else if (strcmp(args->command_arg, "restart") == 0) {
      result_t res_ctx = shog_init_node(args, true);
      node_ctx_t *ctx = PROPAGATE(res_ctx);

      result_t res = restart_node_service(ctx, args);
      PROPAGATE(res);
    } else if (strcmp(args->command_arg, "stop") == 0) {
      result_t res_ctx = shog_init_node(args, true);
      node_ctx_t *ctx = PROPAGATE(res_ctx);

      result_t res = stop_node_service(ctx);
      PROPAGATE(res);
    } else if (strcmp(args->command_arg, "status") == 0) {
      result_t res_ctx = shog_init_node(args, true);
      node_ctx_t *ctx = PROPAGATE(res_ctx);

      result_t res = check_node_service(ctx);
      PROPAGATE(res);
    } else if (strcmp(args->command_arg, "logs") == 0) {
      result_t res_ctx = shog_init_node(args, true);
      node_ctx_t *ctx = PROPAGATE(res_ctx);

      result_t res = print_node_service_logs(ctx);
      PROPAGATE(res);
    } else if (strcmp(args->command_arg, "id") == 0) {
      result_t res_ctx = shog_init_node(args, true);
      node_ctx_t *ctx = PROPAGATE(res_ctx);

      result_t res = node_print_id(ctx);
      PROPAGATE(res);
    } else {
      return ERR("ARGS: invalid node subcommand: '%s'", args->command_arg);
    }
  }

  return OK(NULL);
}
