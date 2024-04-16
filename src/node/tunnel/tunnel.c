/**** tunnel.c ****
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
#include <netlibc/string.h>

#include "../../include/cjson.h"
#include "../../include/shogdb.h"
#include "../../include/sonic.h"
#include "../../json/json.h"
#include "../../utils/utils.h"
#include "../db/db.h"
#include "../manifest/manifest.h"
#include "../node.h"
#include "tunnel.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include <netlibc/mem.h>

void *tunnel_monitor(void *thread_arg) {
  tunnel_monitor_thread_args_t *arg =
      (tunnel_monitor_thread_args_t *)thread_arg;

  for (;;) {
    if (arg->ctx->should_exit) {

      return NULL;
    }

    sleep(5);

    if (arg->ctx->should_exit) {
      return NULL;
    }

    // LOG(INFO, "tunnel monitor!!");

    char url[256];
    sprintf(url, "%s/api/get_manifest", arg->ctx->config->network.public_host);

    sonic_response_t *resp = sonic_get(url);
    if (resp->failed) {
      LOG(INFO, "request failed: %s", resp->error);

      LOG(ERROR, "tunnel host unreachable, restarting tunnel client");

      result_t res_kill = kill_tunnel_client(arg->ctx);
      UNWRAP(res_kill);

      LOG(INFO, "using tunnel port: " U64_FORMAT_SPECIFIER,
          arg->ctx->tunnel_port);

      result_t res = launch_tunnel_client(arg->ctx, arg->ctx->tunnel_port);
      if (is_err(res)) {
        LOG(ERROR, "could not restart tunnel client: %s", res.error_message);
      }

      free_result(res);

    } else {
      char *response_body =
          nmalloc((resp->response_body_size + 1) * sizeof(char));
      strncpy(response_body, resp->response_body, resp->response_body_size);
      response_body[resp->response_body_size] = '\0';

      // LOG(INFO, "tunnel api manifest: %s", response_body);

      nfree(resp->response_body);
      sonic_free_response(resp);
      nfree(response_body);
    }
  }

  return NULL;
}

// FIXME: the process is not terminating
result_t kill_tunnel_client(node_ctx_t *ctx) {
  char node_runtime_path[FILE_PATH_SIZE];
  utils_get_node_runtime_path(ctx, node_runtime_path);

  char tunnel_pid_path[FILE_PATH_SIZE];
  sprintf(tunnel_pid_path, "%s/tunnel_pid.txt", node_runtime_path);

  if (file_exists(tunnel_pid_path)) {
    result_t res_tunnel_pid_str = read_file_to_string(tunnel_pid_path);
    char *tunnel_pid_str = PROPAGATE(res_tunnel_pid_str);

    int pid = atoi(tunnel_pid_str);
    nfree(tunnel_pid_str);

    kill(pid, SIGKILL);

    // u64 kill_count = 0;

    // for (;;) {
    //   int result = kill(pid, 0);

    //   if (result == -1 && errno == ESRCH) {
    //     if (kill_count > 0) {
    //       LOG(INFO, "tunnel process stopped");
    //     }

    //     break;
    //   }

    //   LOG(INFO, "Waiting for tunnel process to exit ...");

    //   sleep(1);

    //   kill_count++;
    // }

    delete_file(tunnel_pid_path);
  }

  return OK(NULL);
}

result_t launch_tunnel_client(node_ctx_t *ctx, u64 custom_port) {
  // result_t res_kill = kill_tunnel_client(ctx);
  // UNWRAP(res_kill);

  char node_runtime_path[FILE_PATH_SIZE];
  utils_get_node_runtime_path(ctx, node_runtime_path);

  char tunnel_logs_path[FILE_PATH_SIZE];
  sprintf(tunnel_logs_path, "%s/tunnel_logs.txt", node_runtime_path);

  pid_t pid = fork();

  ctx->tunnel_client_pid = pid;

  // Create a pipe to send mesages to the db process STDIN
  int node_to_tunnel_pipe[2];
  if (pipe(node_to_tunnel_pipe) == -1) {
    PANIC("could not create node_to_tunnel pipe");
  }

  if (pid == -1) {
    PANIC("could not fork tunnel client process \n");
  } else if (pid == 0) {
    // CHILD PROCESS

    chdir(node_runtime_path);

    int logs_fd = open(tunnel_logs_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (logs_fd == -1) {
      PANIC("could not open tunnel log file");
    }

    // Redirect stdin to the read end of the pipe
    dup2(node_to_tunnel_pipe[0], STDIN_FILENO);

    // Redirect stdout and stderr to the logs file
    dup2(logs_fd, STDOUT_FILENO);
    dup2(logs_fd, STDERR_FILENO);

    char tunnel_pid_path[FILE_PATH_SIZE];
    sprintf(tunnel_pid_path, "%s/tunnel_pid.txt", node_runtime_path);

    pid_t tunnel_pid = getpid();
    char tunnel_pid_str[120];
    sprintf(tunnel_pid_str, "%d", tunnel_pid);
    write_to_file(tunnel_pid_path, tunnel_pid_str, strlen(tunnel_pid_str));

    // Execute the executable

    char tunnel_executable[FILE_PATH_SIZE];
    sprintf(tunnel_executable, "%s/bin/shog_tunnel", ctx->runtime_path);

    char local_port[100];
    sprintf(local_port, "%d", ctx->config->network.port);

    if (custom_port == 0) {
      execlp(tunnel_executable, tunnel_executable, "local", local_port, "--to",
             ctx->config->tunnel.server, NULL);
    } else {
      char custom_port_str[100];
      sprintf(custom_port_str, U64_FORMAT_SPECIFIER, custom_port);

      execlp(tunnel_executable, tunnel_executable, "local", local_port, "--to",
             ctx->config->tunnel.server, "--port", custom_port_str, NULL);
    }

    close(logs_fd);

    char *tunnel_logs = UNWRAP(read_file_to_string(tunnel_logs_path));
    return ERR("error occured while launching tunnel executable. LOGS:\n%s",
               tunnel_logs);
  } else {
    // PARENT PROCESS

    sleep(4);

    int status;
    int child_status = waitpid(ctx->tunnel_client_pid, &status, WNOHANG);

    if (child_status == -1) {
      PANIC("waitpid failed");
    } else if (child_status == 0) {
    } else {
      char *tunnel_logs = UNWRAP(read_file_to_string(tunnel_logs_path));
      return ERR("error occured while launching tunnel executable.\nTUNNEL "
                 "LOGS:\n%s",
                 tunnel_logs);
    }

    char node_runtime_path[FILE_PATH_SIZE];
    utils_get_node_runtime_path(ctx, node_runtime_path);

    char tunnel_logs_path[FILE_PATH_SIZE];
    sprintf(tunnel_logs_path, "%s/tunnel_logs.txt", node_runtime_path);

    char *tunnel_logs = UNWRAP(read_file_to_string(tunnel_logs_path));
    char *last_colon = strrchr(tunnel_logs, ':');
    char *tunnel_port = last_colon + 1;
    tunnel_port[strlen(tunnel_port) - 1] = '\0';

    ctx->tunnel_port = strtoull(tunnel_port, NULL, 10);

    char *new_public_host = string_from("http://", ctx->config->tunnel.server,
                                        ":", tunnel_port, NULL);

    nfree(tunnel_logs);

    ctx->config->network.public_host = new_public_host;
    ctx->manifest->public_host = strdup(new_public_host);

    LOG(INFO, "TUNNELED PUBLIC HOST: %s", ctx->config->network.public_host);
  }

  return OK(NULL);
}

result_t kill_tunnel_server(node_ctx_t *ctx) {
  char node_runtime_path[FILE_PATH_SIZE];
  utils_get_node_runtime_path(ctx, node_runtime_path);

  char tunnel_pid_path[FILE_PATH_SIZE];
  sprintf(tunnel_pid_path, "%s/tunnel_server_pid.txt", node_runtime_path);

  if (file_exists(tunnel_pid_path)) {
    result_t res_tunnel_pid_str = read_file_to_string(tunnel_pid_path);
    char *tunnel_pid_str = PROPAGATE(res_tunnel_pid_str);

    int pid = atoi(tunnel_pid_str);
    nfree(tunnel_pid_str);

    kill(pid, SIGINT);

    u64 kill_count = 0;

    for (;;) {
      int result = kill(pid, 0);

      if (result == -1 && errno == ESRCH) {
        if (kill_count > 0) {
          LOG(INFO, "tunnel server process stopped");
        }

        break;
      }

      LOG(INFO, "Waiting for tunnel server process to exit ...");

      sleep(1);

      kill_count++;
    }

    delete_file(tunnel_pid_path);
  }

  return OK(NULL);
}

void launch_tunnel_server(node_ctx_t *ctx) {
  result_t res_kill = kill_tunnel_server(ctx);
  UNWRAP(res_kill);

  char node_runtime_path[FILE_PATH_SIZE];
  utils_get_node_runtime_path(ctx, node_runtime_path);

  char tunnel_logs_path[FILE_PATH_SIZE];
  sprintf(tunnel_logs_path, "%s/tunnel_server_logs.txt", node_runtime_path);

  pid_t pid = fork();

  ctx->tunnel_server_pid = pid;

  // Create a pipe to send mesages to the db process STDIN
  int node_to_tunnel_pipe[2];
  if (pipe(node_to_tunnel_pipe) == -1) {
    PANIC("could not create node_to_tunnel pipe");
  }

  if (pid == -1) {
    PANIC("could not fork tunnel server process \n");
  } else if (pid == 0) {
    // CHILD PROCESS

    chdir(node_runtime_path);

    int logs_fd = open(tunnel_logs_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (logs_fd == -1) {
      PANIC("could not open tunnel log file");
    }

    // Redirect stdin to the read end of the pipe
    dup2(node_to_tunnel_pipe[0], STDIN_FILENO);

    // Redirect stdout and stderr to the logs file
    dup2(logs_fd, STDOUT_FILENO);
    dup2(logs_fd, STDERR_FILENO);

    char tunnel_pid_path[FILE_PATH_SIZE];
    sprintf(tunnel_pid_path, "%s/tunnel_server_pid.txt", node_runtime_path);

    pid_t tunnel_pid = getpid();
    char tunnel_pid_str[120];
    sprintf(tunnel_pid_str, "%d", tunnel_pid);
    write_to_file(tunnel_pid_path, tunnel_pid_str, strlen(tunnel_pid_str));

    // Execute the executable

    char tunnel_executable[FILE_PATH_SIZE];
    sprintf(tunnel_executable, "%s/bin/shog_tunnel", ctx->runtime_path);

    execlp(tunnel_executable, tunnel_executable, "server", NULL);

    close(logs_fd);

    char *tunnel_logs = UNWRAP(read_file_to_string(tunnel_logs_path));
    PANIC("error occured while launching tunnel executable. LOGS:\n%s",
          tunnel_logs);
  } else {
    // PARENT PROCESS

    sleep(4);

    int status;
    int child_status = waitpid(ctx->tunnel_server_pid, &status, WNOHANG);

    if (child_status == -1) {
      PANIC("waitpid failed");
    } else if (child_status == 0) {
    } else {
      char *tunnel_logs = UNWRAP(read_file_to_string(tunnel_logs_path));
      PANIC("error occured while launching tunnel executable.\nTUNNEL "
            "LOGS:\n%s",
            tunnel_logs);
    }
  }
}
