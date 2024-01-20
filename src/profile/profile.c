/**** profile.c ****
 *
 *  Copyright (c) 2023 Shoggoth Systems
 *
 * Part of the Shoggoth project, under the MIT License.
 * See LICENCE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#include "../include/tuwi.h"
#include "../json/json.h"
#include "../openssl/openssl.h"

#include <assert.h>
#include <stdlib.h>

// FIXME: validate that the Shoggoth ID matches the public key
result_t validate_authorization(char *tmp_path, char *file_path, char *hash,
                                char *public_key, char *fingerprint_str,
                                char *signature) {

  result_t res_calculated_hash = utils_hash_tarball(tmp_path, file_path);
  char *calculated_hash = PROPAGATE(res_calculated_hash);

  if (strcmp(calculated_hash, hash) != 0) {
    char err[256];
    sprintf(err,
            "calculated hash does not match received hash.\ncalculated: "
            "%s\nreceived: %s",
            calculated_hash, hash);
    return ERR(err);
  }

  result_t res_valid_signature =
      openssl_verify_signature(public_key, signature, fingerprint_str);
  int valid_signature = PROPAGATE_INT(res_valid_signature);

  if (!valid_signature) {
    return ERR("received signature is not valid");
  }

  free(calculated_hash);

  return OK(NULL);
}

result_t validate_manifest(char *path) {
  if (!file_exists(path)) {
    return ERR("manifest does not exist");
  }

  // TODO: check if JSON content is valid

  return OK(NULL);
}

result_t validate_git_repo(char *path) {
  if (!dir_exists(path)) {
    char err[256];
    sprintf(err, "repo directory does not exist: %s", path);

    return ERR(err);
  }

  char command[256];
  snprintf(command, sizeof(command),
           "git -C \"%s\" rev-parse --is-inside-work-tree 2>/dev/null", path);

  FILE *pipe = popen(command, "r");
  if (pipe == NULL) {
    perror("popen");
    return ERR("popen failed");
  }

  char buffer[128];
  if (fgets(buffer, sizeof(buffer), pipe) != NULL) {
    // Check the output of the Git command to determine if it's a valid
    // repository.
    if (strcmp(buffer, "true\n") == 0) {
      pclose(pipe);
      return OK(NULL);
    }
  }

  pclose(pipe);

  char err[256];
  sprintf(err, "%s is not a git repository \n", path);

  return ERR(err);
}

result_t validate_resource_group(char *path) {
  if (!dir_exists(path)) {
    return ERR("resource directory does not exist");
  }

  result_t res_files = get_files_and_dirs_list(path);
  files_list_t *files = PROPAGATE(res_files);

  for (u64 i = 0; i < files->files_count; i++) {
    if (strcmp(files->files[i], ".shoggoth") == 0) {
      continue;
    }

    char repo_path[FILE_PATH_SIZE];
    sprintf(repo_path, "%s/%s", path, files->files[i]);

    result_t repo_valid = validate_git_repo(repo_path);

    PROPAGATE(repo_valid);
  }

  free_files_list(files);

  return OK(NULL);
}

result_t validate_profile(char *path) {
  if (!dir_exists(path)) {
    return ERR("profile directory does not exist");
  }

  char metadata_path[FILE_PATH_SIZE];
  sprintf(metadata_path, "%s/.shoggoth", path);

  if (!dir_exists(metadata_path)) {
    return ERR("the directory is not a valid Shoggoth profile: no .shoggoth");
  }

  char manifest_path[FILE_PATH_SIZE];
  sprintf(manifest_path, "%s/manifest.json", metadata_path);

  result_t manifest_valid = validate_manifest(manifest_path);
  PROPAGATE(manifest_valid);

  result_t res_files = get_files_and_dirs_list(path);
  files_list_t *files = PROPAGATE(res_files);

  if (files->files_count < 5) {
    free_files_list(files);

    return ERR(
        "profile directory contains fewer files/directories than necessary");
  } else if (files->files_count > 5) {
    free_files_list(files);

    return ERR(
        "profile directory contains more files/directories than necessary");
  }

  for (u64 i = 0; i < files->files_count; i++) {
    bool invalid_file = false;

    if (strcmp(files->files[i], ".shoggoth") != 0 &&
        strcmp(files->files[i], "code") != 0 &&
        strcmp(files->files[i], "models") != 0 &&
        strcmp(files->files[i], "papers") != 0 &&
        strcmp(files->files[i], "datasets") != 0) {
      invalid_file = true;
    }

    if (invalid_file) {
      char err[256];
      sprintf(err,
              "Invalid Shoggoth profile: directory contains invalid "
              "file/directory `%s`",
              files->files[i]);
      free_files_list(files);

      return ERR(err);
    }
  }

  free_files_list(files);

  char code_path[FILE_PATH_SIZE];
  sprintf(code_path, "%s/code", path);
  result_t code_valid = validate_resource_group(code_path);
  PROPAGATE(code_valid);

  char models_path[FILE_PATH_SIZE];
  sprintf(models_path, "%s/models", path);
  result_t models_valid = validate_resource_group(models_path);
  PROPAGATE(models_valid);

  char papers_path[FILE_PATH_SIZE];
  sprintf(papers_path, "%s/papers", path);
  result_t papers_valid = validate_resource_group(papers_path);
  PROPAGATE(papers_valid);

  char datasets_path[FILE_PATH_SIZE];
  sprintf(datasets_path, "%s/datasets", path);
  result_t datasets_valid = validate_resource_group(datasets_path);
  PROPAGATE(datasets_valid);

  return OK(NULL);
}

result_t clean_ignored_files(char *dir_path) {
  char command[256];
  sprintf(command, "git clean -f -x > /dev/null");

  char prev_cwd[1024];
  getcwd(prev_cwd, sizeof(prev_cwd));

  chdir(dir_path);
  int status = system(command);
  chdir(prev_cwd);

  if (WIFEXITED(status)) {
    int exit_status = WEXITSTATUS(status);
    if (exit_status == 0) {
    } else {
      return ERR("Failed to clean git repo ignored files: %d", exit_status);
    }
  }

  return OK(NULL);
}
