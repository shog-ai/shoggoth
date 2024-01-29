/**** utils.c ****
 *
 *  Copyright (c) 2023 Shoggoth Systems
 *
 * Part of the Shoggoth project, under the MIT License.
 * See LICENSE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#include "utils.h"
#include "../include/tuwi.h"
#include "../node/node.h"

#include <assert.h>
#include <stdlib.h>
#include <uuid/uuid.h>

#ifdef __APPLE__
#define UUID_STR_LEN 37
#endif

char *utils_gen_uuid() {
  uuid_t binuuid;
  uuid_generate_random(binuuid);

  char *uuid = malloc(UUID_STR_LEN * sizeof(char));

  /*
   * Produces a UUID string at uuid consisting of letters
   * whose case depends on the system's locale.
   */
  uuid_unparse(binuuid, uuid);

  return uuid;
}

result_t utils_extract_tarball(char *archive_path, char *destination_path) {
  create_dir(destination_path);

  char command_str[512];

#ifdef __APPLE__
  sprintf(command_str,
          "gtar --strip-components=1 --overwrite "
          "--no-same-owner -xf %s -C %s > /dev/null",
          archive_path, destination_path);
#else
  sprintf(command_str,
          "tar --strip-components=1 --overwrite "
          "--no-same-owner -xf %s -C %s > /dev/null",
          archive_path, destination_path);
#endif

  int status = system(command_str);

  if (WIFEXITED(status)) {
    int exit_status = WEXITSTATUS(status);
    if (exit_status == 0) {
    } else if (exit_status == 1) {
      LOG(ERROR, "untar command succeded but with an exit code: %d",
          exit_status);
    } else {
      return ERR("Failed to untar archive: %d", exit_status);
    }
  }

  return OK(NULL);
}

result_t utils_create_tarball(char *dir_path, char *output_path) {
  char prev_cwd[1024];
  getcwd(prev_cwd, sizeof(prev_cwd));

  assert(dir_exists(dir_path));

  char command_str[512];
#ifdef __APPLE__
  sprintf(command_str,
          "gtar --sort=name  --preserve-permissions --group=shog --owner=shog "
          "--mtime='UTC 2019-01-01' "
          "-cf %s . > /dev/null",
          output_path);

#else
  sprintf(command_str,
          "tar --sort=name  --preserve-permissions --group=shog --owner=shog "
          "--mtime='UTC 2019-01-01' "
          "-cf %s . > /dev/null",
          output_path);

#endif

  chdir(dir_path);
  int status = system(command_str);
  chdir(prev_cwd);

  if (WIFEXITED(status)) {
    int exit_status = WEXITSTATUS(status);
    if (exit_status == 0) {
    } else if (exit_status == 1) {
      LOG(ERROR, "tar command succeded but with an exit code: %d", exit_status);
    } else {
      return ERR("Failed to create tar archive: %d", exit_status);
    }
  }

  return OK(NULL);
}

result_t utils_hash_tarball(char *tmp_path, char *tarball_path) {
  char *uuid = utils_gen_uuid();

  char hash_tmp_path[FILE_PATH_SIZE];
  sprintf(hash_tmp_path, "%s/%s", tmp_path, uuid);

  free(uuid);

  result_t res_untar = utils_extract_tarball(tarball_path, hash_tmp_path);
  PROPAGATE(res_untar);

  char prev_cwd[1024];
  getcwd(prev_cwd, sizeof(prev_cwd));

  char command_str[512];
  sprintf(command_str,
          "find . -type f -exec sha256sum {} \\; | sort -k 1 | sha256sum");

  chdir(hash_tmp_path);

  FILE *pipe = popen(command_str, "r");
  if (!pipe) {
    perror("popen");
    return ERR("popen failed when calculating tarball hash");
  }

  char result[1024] = "";
  char buffer[256];

  while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
    strncat(result, buffer, sizeof(result) - strlen(result) - 1);
  }

  int status = pclose(pipe);

  chdir(prev_cwd);

  result_t res_delete = delete_dir(hash_tmp_path);
  PROPAGATE(res_delete);

  if (WIFEXITED(status)) {
    int exit_status = WEXITSTATUS(status);
    if (exit_status == 0) {
    } else {
      return ERR("Failed to calculate tarball hash: %d", exit_status);
    }
  }

  result[64] = '\0';

  char *res = strdup(result);

  // LOG(INFO, "CHECKSUM: %s", res);

  return OK(res);
}

result_t utils_clear_dir_timestamps(char *dir_path) {
  char prev_cwd[1024];
  getcwd(prev_cwd, sizeof(prev_cwd));

  char command_str[512];
  sprintf(command_str, "find ./ -exec touch -t 200001011200.00 {} \\;");

  chdir(dir_path);
  int status = system(command_str);
  chdir(prev_cwd);

  if (WIFEXITED(status)) {
    int exit_status = WEXITSTATUS(status);
    if (exit_status == 0) {
    } else {
      return ERR("Failed to reset timestamps: %d", exit_status);
    }
  }

  return OK(NULL);
}

result_t utils_clear_dir_permissions(char *dir_path) {
  char command_str[512];
  sprintf(command_str, "find %s -type f -exec chmod 664 {} \\;", dir_path);

  int status = system(command_str);

  if (WIFEXITED(status)) {
    int exit_status = WEXITSTATUS(status);
    if (exit_status == 0) {
    } else {
      return ERR("Failed to reset permissions: %d", exit_status);
    }
  }

  return OK(NULL);
}

/****U
 * because generating a new RSA key with openssl adds a line at the top and
 * the bottom indicating the start and end of the key (see
 * /deploy/1/public.txt), this function is used to extract the actual key from
 * the string. It also removes the newlines
 ****/
char *utils_strip_public_key(const char *input) {
  char *strip_buf = NULL;

  u32 i = 0;
  u32 k = 0;

  for (; i < strlen(input); i++) {
    if (i > 35 && i < (strlen(input) - 35) && input[i] == '\n') {
      continue;
    }

    strip_buf = realloc(strip_buf, (k + 1) * sizeof(char));

    strip_buf[k] = input[i];

    k++;
  }

  strip_buf = realloc(strip_buf, (k + 1) * sizeof(char));
  strip_buf[k] = '\0';

  return strip_buf;
}

/****
 * utility function to verify that the public and private keys exist in the
 * specified path
 *
 ****/
bool utils_keys_exist(char *keys_path) {
  char private_key_path[FILE_PATH_SIZE];
  sprintf(private_key_path, "%s/private.txt", keys_path);

  char public_key_path[FILE_PATH_SIZE];
  sprintf(public_key_path, "%s/public.txt", keys_path);

  if (file_exists(public_key_path) && file_exists(private_key_path)) {
    return true;
  } else {
    return false;
  }
}

/****
 * utility function to verify that all the necessary directories and
 * subdirectories used by the node runtime exist. If they don't exist, it
 * creates them
 *
 ****/
void utils_verify_node_runtime_dirs(node_ctx_t *ctx) {
  char runtime_path[FILE_PATH_SIZE];
  strcpy(runtime_path, ctx->runtime_path);

  char node_runtime_path[FILE_PATH_SIZE];
  utils_get_node_runtime_path(ctx, node_runtime_path);

  char node_explorer_path[FILE_PATH_SIZE];
  utils_get_node_explorer_path(ctx, node_explorer_path);

  char node_tmp_path[FILE_PATH_SIZE];
  utils_get_node_tmp_path(ctx, node_tmp_path);

  char node_update_path[FILE_PATH_SIZE];
  utils_get_node_update_path(ctx, node_update_path);

  assert(runtime_path != NULL);

  if (!dir_exists(runtime_path)) {
    create_dir(runtime_path);
  }

  if (!dir_exists(node_runtime_path)) {
    create_dir(node_runtime_path);
  }

  if (!dir_exists(node_explorer_path)) {
    create_dir(node_explorer_path);
  }

  if (!dir_exists(node_tmp_path)) {
    create_dir(node_tmp_path);
  }

  if (!dir_exists(node_update_path)) {
    create_dir(node_update_path);
  }
}

/****
 * utility function to get the default runtime path which should be
 * $HOME/shoggoth
 *
 ****/
int utils_get_default_runtime_path(char runtime_path[]) {
#ifdef _WIN32
  const char *home_path = getenv("USERPROFILE");
#else
  const char *home_path = getenv("HOME");
#endif

  if (home_path == NULL) {
    return 1;
  }

  char *sub_dir = "/shoggoth";
  sprintf(runtime_path, "%s%s", home_path, sub_dir);

  return 0;
}

/****U
 * utility function to derive the node runtime path from the already set runtime
 * path in the ctx. If the default runtime path was used, the node runtime
 * path should be $HOME/shoggoth/node
 *
 ****/
void utils_get_node_runtime_path(node_ctx_t *ctx, char node_runtime_path[]) {
  char runtime_path[FILE_PATH_SIZE];
  strcpy(runtime_path, ctx->runtime_path);

  char *relative_path = "/node";

  sprintf(node_runtime_path, "%s%s", runtime_path, relative_path);
}

/****U
 * utility function to derive the node explorer runtime path from the already
 * set runtime path in the ctx. If the default runtime path was used, the
 * explorer runtime path should be $HOME/shoggoth/node/explorer
 *
 ****/
void utils_get_node_explorer_path(node_ctx_t *ctx, char node_explorer_path[]) {
  char node_runtime_path[FILE_PATH_SIZE];
  utils_get_node_runtime_path(ctx, node_runtime_path);

  char *relative_path = "/explorer";

  sprintf(node_explorer_path, "%s%s", node_runtime_path, relative_path);
}

/****U
 * utility function to derive the node tmp runtime path (used for temporary
 * files) from the already set runtime path in the ctx. If the default runtime
 * path was used, the tmp runtime path should be $HOME/shoggoth/node/tmp
 *
 ****/
void utils_get_node_tmp_path(node_ctx_t *ctx, char node_tmp_path[]) {
  char node_runtime_path[FILE_PATH_SIZE];
  utils_get_node_runtime_path(ctx, node_runtime_path);

  char *relative_path = "/tmp";

  sprintf(node_tmp_path, "%s%s", node_runtime_path, relative_path);
}

void utils_get_node_update_path(node_ctx_t *ctx, char node_update_path[]) {
  char node_runtime_path[FILE_PATH_SIZE];
  utils_get_node_runtime_path(ctx, node_runtime_path);

  char *relative_path = "/update";

  sprintf(node_update_path, "%s%s", node_runtime_path, relative_path);
}
