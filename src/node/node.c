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
  // chdir(ctx->runtime_path);

  char node_runtime_path[FILE_PATH_SIZE];
  utils_get_node_runtime_path(ctx, node_runtime_path);

  char *keys_path = string_from(node_runtime_path, "/keys", NULL);

  char *private_key_path = string_from(keys_path, "/private.txt", NULL);
  char *public_key_path = string_from(keys_path, "/public.txt", NULL);

  if (!dir_exists(keys_path)) {
    create_dir(keys_path);
  }

  if (!utils_keys_exist(keys_path)) {
    result_t res =
        generate_node_key_pair(keys_path, public_key_path, private_key_path);
    PROPAGATE(res);
  }

  free(keys_path);

  char *pins_path = string_from(node_runtime_path, "/pins", NULL);
  if (!dir_exists(pins_path)) {
    create_dir(pins_path);
  }
  free(pins_path);

  char *config_path = NULL;

  if (args->set_config) {
    config_path = string_from(args->config_path, NULL);
  } else {
    config_path = string_from(node_runtime_path, "/config.toml", NULL);
  }

  if (!file_exists(config_path)) {
    return ERR("Config file `%s` does not exist", config_path);
  }

  result_t res_config_str = read_file_to_string(config_path);
  char *config_str = PROPAGATE(res_config_str);

  free(config_path);

  result_t res_config = toml_string_to_node_config(config_str);
  node_config_t *config = PROPAGATE(res_config);

  free(config_str);

  ctx->config = config;

  result_t res_public_key_string = read_file_to_string(public_key_path);
  char *public_key_string = PROPAGATE(res_public_key_string);

  free(public_key_path);
  free(private_key_path);

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

  // Send a kill signal to the db process
  if (kill(node_global_ctx->db_pid, SIGTERM) == -1) {
    PANIC("kill process failed");
  }

  // Wait for the child process to exit
  int status;
  pid_t exited_pid = waitpid(node_global_ctx->db_pid, &status, 0);

  if (exited_pid == -1) {
    PANIC("waitpid failed");
  }

  if (WIFEXITED(status)) {
  } else {
    LOG(ERROR, "db child process did not exit normally");
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

  dht_thread_args_t dht_thread_args = {.ctx = ctx};
  if (pthread_create(&dht_thread, NULL, dht_updater, &dht_thread_args) != 0) {
    PANIC("could not spawn dht thread");
  }

  pthread_join(server_thread, NULL);
  pthread_join(dht_thread, NULL);

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

    if (!dir_exists(ctx->runtime_path)) {
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

void sigchld_handler(int signo) {
  (void)signo; // To silence the unused parameter warning

  // Reap all child processes
  while (waitpid(-1, NULL, WNOHANG) > 0)
    ;
}

result_t start_node_monitor(node_ctx_t *ctx, args_t *args) {
  ctx = (void *)ctx;

  LOG(INFO, "Starting node monitor");

  pid_t pid = fork();

  // Create a pipe to send mesages to the child process STDIN
  int parent_to_child_pipe[2];
  if (pipe(parent_to_child_pipe) == -1) {
    PANIC("could not create parent_to_child pipe");
  }

  if (pid == -1) {
    PANIC("could not fork monitor process \n");
  } else if (pid == 0) {
    // CHILD PROCESS

    // Set up the SIGCHLD signal handler
    // prevents node process from becoming a zombie
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigaction(SIGCHLD, &sa, NULL);

    char node_runtime_path[FILE_PATH_SIZE];
    utils_get_node_runtime_path(ctx, node_runtime_path);

    char *monitor_logs_path =
        string_from(node_runtime_path, "/monitor_logs.txt", NULL);

    int logs_fd = open(monitor_logs_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (logs_fd == -1) {
      PANIC("could not open node monitor log file");
    }

    free(monitor_logs_path);

    // Redirect stdin to the read end of the pipe
    dup2(parent_to_child_pipe[0], STDIN_FILENO);

    // Redirect stdout and stderr to the logs file
    dup2(logs_fd, STDOUT_FILENO);
    dup2(logs_fd, STDERR_FILENO);

    char *node_pid_path =
        string_from(node_runtime_path, "/node_service_pid.txt", NULL);

    for (;;) {
      sleep(5);

      result_t res_node_pid_str = read_file_to_string(node_pid_path);

      char *node_pid_str = UNWRAP(res_node_pid_str);

      if (node_pid_str == NULL) {
        PANIC("Node service is not running (pid file not readable)");
      }

      int pid = atoi(node_pid_str);
      free(node_pid_str);

      int result = kill(pid, 0);

      if (result == 0) {
        LOG(INFO, "Node is running\n");
      } else if (result == -1) {
        LOG(INFO, "Node is not running\n");

        start_node_service(ctx, args, false);
      }
    }
  }

  return OK(NULL);
}

result_t start_node_service(node_ctx_t *ctx, args_t *args, bool monitor) {
  LOG(INFO, "Starting node as a service");

  char node_runtime_path[FILE_PATH_SIZE];
  utils_get_node_runtime_path(ctx, node_runtime_path);

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

    char *node_config_path = NULL;
    if (args->set_config) {
      node_config_path = string_from(args->config_path, NULL);
    } else {
      node_config_path =
          string_from(ctx->runtime_path, "/node/config.toml", NULL);
    }

    char *node_logs_path =
        string_from(node_runtime_path, "/node_service_logs.txt", NULL);

    char *node_pid_path =
        string_from(node_runtime_path, "/node_service_pid.txt", NULL);

    pid_t node_pid = getpid();
    char node_pid_str[120];
    sprintf(node_pid_str, "%d", node_pid);
    write_to_file(node_pid_path, node_pid_str, strlen(node_pid_str));

    free(node_pid_path);

    int logs_fd = open(node_logs_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (logs_fd == -1) {
      PANIC("could not open node log file");
    }

    free(node_logs_path);

    // Redirect stdin to the read end of the pipe
    dup2(parent_to_child_pipe[0], STDIN_FILENO);

    // Redirect stdout and stderr to the logs file
    dup2(logs_fd, STDOUT_FILENO);
    dup2(logs_fd, STDERR_FILENO);

    // Execute the node executable

    char *node_executable = string_from(ctx->runtime_path, "/bin/shog", NULL);

    char *config_arg = strdup(node_config_path);

    free(node_config_path);

    char *node_runtime_arg = strdup(ctx->runtime_path);

    execlp("nohup", "nohup", node_executable, "node", "run", "-c", config_arg,
           "-r", node_runtime_arg, NULL);

    free(config_arg);
    free(node_runtime_arg);

    close(logs_fd);
    PANIC("error occured while launching node executable \n");
  }

  if (monitor) {
    UNWRAP(start_node_monitor(ctx, args));
  }

  return OK(NULL);
}

result_t check_node_service(node_ctx_t *ctx) {
  char node_runtime_path[FILE_PATH_SIZE];
  utils_get_node_runtime_path(ctx, node_runtime_path);

  char *node_pid_path =
      string_from(node_runtime_path, "/node_service_pid.txt", NULL);

  result_t res_node_pid_str = read_file_to_string(node_pid_path);
  free(node_pid_path);

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

  char *node_pid_path =
      string_from(node_runtime_path, "/node_service_pid.txt", NULL);

  result_t res_node_pid_str = read_file_to_string(node_pid_path);

  char *node_pid_str = PROPAGATE(res_node_pid_str);

  if (node_pid_str == NULL) {
    return ERR("Node service is not running");
  }

  result_t res_delete = delete_file(node_pid_path);
  PROPAGATE(res_delete);

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

  delete_file(node_pid_path);

  free(node_pid_path);

  LOG(INFO, "Node service stopped");

  return OK(NULL);
}

result_t print_node_service_logs(node_ctx_t *ctx) {
  char node_runtime_path[FILE_PATH_SIZE];
  utils_get_node_runtime_path(ctx, node_runtime_path);

  char *node_logs_path =
      string_from(node_runtime_path, "/node_service_logs.txt", NULL);

  result_t res_node_logs_str = read_file_to_string(node_logs_path);
  free(node_logs_path);

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

  char *node_pid_path =
      string_from(node_runtime_path, "/node_service_pid.txt", NULL);

  result_t res_node_pid_str = read_file_to_string(node_pid_path);

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

    delete_file(node_pid_path);

    free(node_pid_path);

    LOG(INFO, "Node service stopped");

    start_node_service(ctx, args, true);
  } else if (result == -1) {
    printf("Node is not running\n");
  }

  return OK(NULL);
}

result_t node_backup(node_ctx_t *ctx) {
  LOG(INFO, "Starting node backup ...");

  char *node_runtime_path = string_from(ctx->runtime_path, "/node", NULL);
  char *backup_path = string_from(node_runtime_path, "/tmp/backup", NULL);

  create_dir(backup_path);

  char *config_path = string_from(node_runtime_path, "/config.toml", NULL);
  char *backup_config_path = string_from(backup_path, "/config.toml", NULL);
  UNWRAP(copy_file(config_path, backup_config_path));
  free(config_path);
  free(backup_config_path);

  char *dbconfig_path = string_from(node_runtime_path, "/dbconfig.toml", NULL);
  char *backup_dbconfig_path = string_from(backup_path, "/dbconfig.toml", NULL);
  UNWRAP(copy_file(dbconfig_path, backup_dbconfig_path));
  free(dbconfig_path);
  free(backup_dbconfig_path);

  char *dbsave_path = string_from(node_runtime_path, "/save.sdb", NULL);
  char *backup_dbsave_path = string_from(backup_path, "/save.sdb", NULL);
  UNWRAP(copy_file(dbsave_path, backup_dbsave_path));
  free(dbsave_path);
  free(backup_dbsave_path);

  char *pins_path = string_from(node_runtime_path, "/pins", NULL);
  char *backup_pins_path = string_from(backup_path, "/pins", NULL);
  UNWRAP(copy_dir(pins_path, backup_pins_path));
  free(pins_path);
  free(backup_pins_path);

  char *tarball_path = string_from(node_runtime_path, "/backup.tar", NULL);
  UNWRAP(utils_create_tarball(backup_path, tarball_path));
  free(tarball_path);

  delete_dir(backup_path);

  free(node_runtime_path);
  free(backup_path);

  LOG(INFO, "Node backup finished");

  return OK(NULL);
}

result_t node_restore(node_ctx_t *ctx) {
  LOG(INFO, "Starting node restore ...");

  char *node_runtime_path = string_from(ctx->runtime_path, "/node", NULL);
  char *backup_path = string_from(node_runtime_path, "/tmp/backup", NULL);

  char *tarball_path = string_from(node_runtime_path, "/backup.tar", NULL);
  UNWRAP(utils_extract_tarball(tarball_path, backup_path));
  free(tarball_path);

  char *config_path = string_from(node_runtime_path, "/config.toml", NULL);
  char *backup_config_path = string_from(backup_path, "/config.toml", NULL);
  UNWRAP(copy_file(backup_config_path, config_path));
  free(config_path);
  free(backup_config_path);

  char *dbconfig_path = string_from(node_runtime_path, "/dbconfig.toml", NULL);
  char *backup_dbconfig_path = string_from(backup_path, "/dbconfig.toml", NULL);
  UNWRAP(copy_file(backup_dbconfig_path, dbconfig_path));
  free(dbconfig_path);
  free(backup_dbconfig_path);

  char *dbsave_path = string_from(node_runtime_path, "/save.sdb", NULL);
  char *backup_dbsave_path = string_from(backup_path, "/save.sdb", NULL);
  UNWRAP(copy_file(backup_dbsave_path, dbsave_path));
  free(dbsave_path);
  free(backup_dbsave_path);

  char *pins_path = string_from(node_runtime_path, "/pins", NULL);
  char *backup_pins_path = string_from(backup_path, "/pins", NULL);
  UNWRAP(copy_dir(backup_pins_path, pins_path));
  free(pins_path);
  free(backup_pins_path);

  free(node_runtime_path);
  free(backup_path);

  LOG(INFO, "Node restore finished");

  return OK(NULL);
}

result_t shoggoth_id_from_hash(char *hash) {
  char id_string[512];

  strcpy(id_string, "SHOG");
  strcat(id_string, hash);

  char *id = strdup(id_string);
  return OK(id);
}

result_t node_unpin_resource(node_ctx_t *ctx, char *shoggoth_id) {
  LOG(INFO, "unpinning resource `%s`", shoggoth_id);

  char node_runtime_path[FILE_PATH_SIZE];
  utils_get_node_runtime_path(ctx, node_runtime_path);

  char *pins_path = string_from(node_runtime_path, "/pins", NULL);
  char *destination_path = string_from(pins_path, "/", shoggoth_id, NULL);

  bool exists = file_exists(destination_path);

  if (exists) {
    delete_file(destination_path);

    result_t res_add = db_pins_remove_resource(ctx, shoggoth_id);
    PROPAGATE(res_add);
  } else {
    return ERR("resource not found");
  }

  LOG(INFO, "resource unpinned successfully");

  return OK(NULL);
}

result_t node_pin_resource(node_ctx_t *ctx, char *file_path, char *label) {
  LOG(INFO, "pinning file `%s` with label `%s`", file_path, label);

  result_t res_mapping = map_file(file_path);
  file_mapping_t *mapping = PROPAGATE(res_mapping);

  result_t res_hash = openssl_hash_data(
      mapping->content, (unsigned long long)mapping->info.st_size);
  char *hash = PROPAGATE(res_hash);

  unmap_file(mapping);

  result_t res_id = shoggoth_id_from_hash(hash);
  char *id = PROPAGATE(res_id);

  char node_runtime_path[FILE_PATH_SIZE];
  utils_get_node_runtime_path(ctx, node_runtime_path);

  char *pins_path = string_from(node_runtime_path, "/pins", NULL);
  char *destination_path = string_from(pins_path, "/", id, NULL);

  copy_file(file_path, destination_path);

  result_t res_add = db_pins_add_resource(ctx, id, label);
  PROPAGATE(res_add);

  LOG(INFO, "resource pinned successfully");
  LOG(INFO, "Shoggoth ID: %s", id);

  return OK(NULL);
}

void clone_response_callback(char *data, u64 size, void *user_pointer) {
  char *tmp_file_path = (char *)user_pointer;

  result_t res = append_to_file(tmp_file_path, data, size);
  UNWRAP(res);
}

result_t node_clone_resource(node_ctx_t *ctx, char *url, char *label) {
  LOG(INFO, "cloning resource `%s`", url);

  char tmp_path[FILE_PATH_SIZE];
  utils_get_node_tmp_path(ctx, tmp_path);

  char tmp_file_path[FILE_PATH_SIZE];
  sprintf(tmp_file_path, "%s/clone.tmp", tmp_path);

  sonic_request_t *req = sonic_new_request(METHOD_GET, url);

  char *user_ptr = strdup(tmp_file_path);

  sonic_request_set_response_callback(req, clone_response_callback, user_ptr);

  sonic_response_t *resp = sonic_send_request(req);

  free(user_ptr);

  if (resp->failed) {
    return ERR("CLONE FAILED: %s", sonic_get_error());
  } else {
    if (resp->status != STATUS_200) {
      return ERR("CLONE FAILED: status was not OK");
    }
  }

  UNWRAP(node_pin_resource(ctx, tmp_file_path, label));

  delete_file(tmp_file_path);

  LOG(INFO, "resource cloned successfully");

  return OK(NULL);
}

/****
 * handles a node session.
 *
 ****/
result_t handle_node_session(args_t *args) {
  if (strcmp(args->command, "run") == 0) {
    result_t res_ctx = shog_init_node(args, true);
    node_ctx_t *ctx = PROPAGATE(res_ctx);

    result_t res = run_node_server(ctx);
    PROPAGATE(res);
  } else if (strcmp(args->command, "start") == 0) {
    result_t res_ctx = shog_init_node(args, true);
    node_ctx_t *ctx = PROPAGATE(res_ctx);

    result_t res = start_node_service(ctx, args, true);
    PROPAGATE(res);
  } else if (strcmp(args->command, "restart") == 0) {
    result_t res_ctx = shog_init_node(args, true);
    node_ctx_t *ctx = PROPAGATE(res_ctx);

    result_t res = restart_node_service(ctx, args);
    PROPAGATE(res);
  } else if (strcmp(args->command, "stop") == 0) {
    result_t res_ctx = shog_init_node(args, true);
    node_ctx_t *ctx = PROPAGATE(res_ctx);

    result_t res = stop_node_service(ctx);
    PROPAGATE(res);
  } else if (strcmp(args->command, "status") == 0) {
    result_t res_ctx = shog_init_node(args, true);
    node_ctx_t *ctx = PROPAGATE(res_ctx);

    result_t res = check_node_service(ctx);
    PROPAGATE(res);
  } else if (strcmp(args->command, "logs") == 0) {
    result_t res_ctx = shog_init_node(args, true);
    node_ctx_t *ctx = PROPAGATE(res_ctx);

    result_t res = print_node_service_logs(ctx);
    PROPAGATE(res);
  } else if (strcmp(args->command, "id") == 0) {
    result_t res_ctx = shog_init_node(args, true);
    node_ctx_t *ctx = PROPAGATE(res_ctx);

    result_t res = node_print_id(ctx);
    PROPAGATE(res);
  } else if (strcmp(args->command, "pin") == 0) {
    if (args->has_command_arg) {
      if (args->has_subcommand_arg) {

        result_t res_ctx = shog_init_node(args, true);
        node_ctx_t *ctx = PROPAGATE(res_ctx);

        result_t res =
            node_pin_resource(ctx, args->command_arg, args->subcommand_arg);
        PROPAGATE(res);
      } else {
        EXIT(1, "no label was provided to `pin` command");
      }
    } else {
      EXIT(1, "no file was provided to `pin` command");
    }
  } else if (strcmp(args->command, "unpin") == 0) {
    if (args->has_command_arg) {
      result_t res_ctx = shog_init_node(args, true);
      node_ctx_t *ctx = PROPAGATE(res_ctx);

      result_t res = node_unpin_resource(ctx, args->command_arg);
      PROPAGATE(res);
    } else {
      EXIT(1, "no Shoggoth ID was provided to `unpin` command");
    }
  } else if (strcmp(args->command, "clone") == 0) {
    if (args->has_command_arg) {
      if (args->has_subcommand_arg) {

        result_t res_ctx = shog_init_node(args, true);
        node_ctx_t *ctx = PROPAGATE(res_ctx);

        result_t res =
            node_clone_resource(ctx, args->command_arg, args->subcommand_arg);
        PROPAGATE(res);

      } else {
        EXIT(1, "no label was provided to `clone` command");
      }
    } else {
      EXIT(1, "no URL was provided to `clone` command");
    }
  } else if (strcmp(args->command, "backup") == 0) {
    result_t res_ctx = shog_init_node(args, true);
    node_ctx_t *ctx = PROPAGATE(res_ctx);

    result_t res = node_backup(ctx);
    PROPAGATE(res);
  } else if (strcmp(args->command, "restore") == 0) {
    result_t res_ctx = shog_init_node(args, true);
    node_ctx_t *ctx = PROPAGATE(res_ctx);

    result_t res = node_restore(ctx);
    PROPAGATE(res);
  } else {
    return ERR("ARGS: invalid command: '%s'", args->command);
  }

  return OK(NULL);
}
