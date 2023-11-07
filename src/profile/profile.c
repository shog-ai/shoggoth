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

/****U
 * creates a new profile directory and its subdirectories in the current
 * working directory
 *
 ****/
result_t client_init_profile(client_manifest_t *manifest) {
  LOG(INFO, "Creating new profile ...");

  mkdir("shoggoth-profile", 0777);

  mkdir("./shoggoth-profile/.shoggoth", 0777);

  mkdir("./shoggoth-profile/code", 0777);
  mkdir("./shoggoth-profile/models", 0777);
  mkdir("./shoggoth-profile/datasets", 0777);
  mkdir("./shoggoth-profile/papers", 0777);

  create_file("./shoggoth-profile/.shoggoth/manifest.json");

  result_t res_manifest_json = json_client_manifest_to_json(*manifest);
  json_t *manifest_json = PROPAGATE(res_manifest_json);

  char *manifest_str = json_to_string(manifest_json);
  free_json(manifest_json);

  result_t res = write_to_file("./shoggoth-profile/.shoggoth/manifest.json",
                               manifest_str, strlen(manifest_str));
  PROPAGATE(res);

  free(manifest_str);

  LOG(INFO, "New profile created");

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

void free_resource_fingerprint(resource_fingerprint_t *fingerprint) {
  free(fingerprint->public_key);
  free(fingerprint->shoggoth_id);
  free(fingerprint->resource_name);
  free(fingerprint->hash);

  free(fingerprint);
}

void free_group_fingerprint(group_fingerprint_t *fingerprint) {
  free(fingerprint->public_key);
  free(fingerprint->shoggoth_id);
  free(fingerprint->group_name);
  free(fingerprint->hash);

  free(fingerprint);
}

void free_fingerprint(fingerprint_t *fingerprint) {
  free(fingerprint->public_key);
  free(fingerprint->shoggoth_id);
  free(fingerprint->hash);
  free(fingerprint->timestamp);

  free(fingerprint);
}

result_t generate_resource_signatures(client_ctx_t *ctx, char *group_name) {
  char tmp_path[FILE_PATH_SIZE];
  utils_get_client_tmp_path(ctx, tmp_path);

  char tmp_dir_path[FILE_PATH_SIZE];
  sprintf(tmp_dir_path, "%s/profile", tmp_path);

  char group_tmp_dir_path[FILE_PATH_SIZE];
  sprintf(group_tmp_dir_path, "%s/%s", tmp_dir_path, group_name);

  char group_metadata_path[FILE_PATH_SIZE];
  sprintf(group_metadata_path, "%s/.shoggoth", group_tmp_dir_path);
  create_dir(group_metadata_path);

  char group_fingerprints_path[FILE_PATH_SIZE];
  sprintf(group_fingerprints_path, "%s/fingerprints", group_metadata_path);
  create_dir(group_fingerprints_path);

  result_t res_files_list = get_files_and_dirs_list(group_tmp_dir_path);
  files_list_t *files_list = PROPAGATE(res_files_list);

  for (u64 i = 0; i < files_list->files_count; i++) {
    if (strcmp(files_list->files[i], ".shoggoth") == 0) {
      continue;
    }

    char resource_dir_path[FILE_PATH_SIZE];
    sprintf(resource_dir_path, "%s/%s", group_tmp_dir_path,
            files_list->files[i]);

    char resource_tmp_tarball_path[FILE_PATH_SIZE];
    sprintf(resource_tmp_tarball_path, "%s/profile.%s.%s.tar", tmp_path,
            group_name, files_list->files[i]);

    utils_create_tarball(resource_dir_path, resource_tmp_tarball_path);

    result_t res_file_hash =
        utils_hash_tarball(tmp_path, resource_tmp_tarball_path);
    char *file_hash = PROPAGATE(res_file_hash);

    resource_fingerprint_t *fingerprint =
        calloc(1, sizeof(resource_fingerprint_t));
    fingerprint->shoggoth_id = strdup(ctx->manifest->shoggoth_id);
    fingerprint->public_key = strdup(ctx->manifest->public_key);
    fingerprint->hash = strdup(file_hash);
    fingerprint->resource_name = strdup(files_list->files[i]);

    result_t res_fingerprint_json =
        json_resource_fingerprint_to_json(*fingerprint);
    json_t *fingerprint_json = PROPAGATE(res_fingerprint_json);

    char *fingerprint_str = json_to_string(fingerprint_json);
    free_json(fingerprint_json);
    free_resource_fingerprint(fingerprint);

    char private_key_path[FILE_PATH_SIZE];
    utils_get_client_runtime_path(ctx, private_key_path);
    strcat(private_key_path, "/keys/private.txt");

    result_t res_fingerprint_signature =
        openssl_sign_data(private_key_path, fingerprint_str);
    char *fingerprint_signature = PROPAGATE(res_fingerprint_signature);

    char resource_metadata_path[FILE_PATH_SIZE];
    sprintf(resource_metadata_path, "%s/%s", group_fingerprints_path,
            files_list->files[i]);
    create_dir(resource_metadata_path);

    char fingerprint_path[FILE_PATH_SIZE];
    sprintf(fingerprint_path, "%s/fingerprint.json", resource_metadata_path);
    write_to_file(fingerprint_path, fingerprint_str, strlen(fingerprint_str));

    char signature_path[FILE_PATH_SIZE];
    sprintf(signature_path, "%s/signature.txt", resource_metadata_path);
    write_to_file(signature_path, fingerprint_signature,
                  strlen(fingerprint_signature));

    delete_file(resource_tmp_tarball_path);

    free(fingerprint_str);
    free(fingerprint_signature);
    free(file_hash);
  }

  free_files_list(files_list);

  char group_tmp_tarball_path[FILE_PATH_SIZE];
  sprintf(group_tmp_tarball_path, "%s/profile.%s.tar", tmp_path, group_name);

  result_t res_tarball =
      utils_create_tarball(group_tmp_dir_path, group_tmp_tarball_path);
  PROPAGATE(res_tarball);

  result_t res_file_hash = utils_hash_tarball(tmp_path, group_tmp_tarball_path);
  char *file_hash = PROPAGATE(res_file_hash);

  group_fingerprint_t *fingerprint = calloc(1, sizeof(group_fingerprint_t));

  fingerprint->shoggoth_id = strdup(ctx->manifest->shoggoth_id);
  fingerprint->public_key = strdup(ctx->manifest->public_key);
  fingerprint->hash = strdup(file_hash);
  fingerprint->group_name = strdup(group_name);

  result_t res_fingerprint_json = json_group_fingerprint_to_json(*fingerprint);
  json_t *fingerprint_json = PROPAGATE(res_fingerprint_json);

  char *fingerprint_str = json_to_string(fingerprint_json);
  free_json(fingerprint_json);
  free_group_fingerprint(fingerprint);

  char private_key_path[FILE_PATH_SIZE];
  utils_get_client_runtime_path(ctx, private_key_path);
  strcat(private_key_path, "/keys/private.txt");

  result_t res_fingerprint_signature =
      openssl_sign_data(private_key_path, fingerprint_str);
  char *fingerprint_signature = PROPAGATE(res_fingerprint_signature);

  char fingerprint_path[FILE_PATH_SIZE];
  sprintf(fingerprint_path, "%s/fingerprint.json", group_metadata_path);
  result_t res_write =
      write_to_file(fingerprint_path, fingerprint_str, strlen(fingerprint_str));
  PROPAGATE(res_write);

  char signature_path[FILE_PATH_SIZE];
  sprintf(signature_path, "%s/signature.txt", group_metadata_path);
  res_write = write_to_file(signature_path, fingerprint_signature,
                            strlen(fingerprint_signature));
  UNWRAP(res_write);

  result_t res_delete = delete_file(group_tmp_tarball_path);
  PROPAGATE(res_delete);

  free(fingerprint_str);
  free(fingerprint_signature);
  free(file_hash);

  return OK(NULL);
}

result_t generate_group_signatures(client_ctx_t *ctx) {
  result_t res_gen = generate_resource_signatures(ctx, "code");
  PROPAGATE(res_gen);

  res_gen = generate_resource_signatures(ctx, "models");
  PROPAGATE(res_gen);

  res_gen = generate_resource_signatures(ctx, "datasets");
  PROPAGATE(res_gen);

  res_gen = generate_resource_signatures(ctx, "papers");
  PROPAGATE(res_gen);

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

void copy_git_repo(client_ctx_t *ctx, char *source_path,
                   char *destination_path) {

  ctx = (void *)ctx;

  copy_dir(source_path, destination_path);

  clean_ignored_files(destination_path);
}

result_t copy_group(client_ctx_t *ctx, char *group_name, char *source_path,
                    char *destination_path) {
  char group_path[FILE_PATH_SIZE];
  sprintf(group_path, "%s/%s", source_path, group_name);

  char group_dest_path[FILE_PATH_SIZE];
  sprintf(group_dest_path, "%s/%s", destination_path, group_name);

  create_dir(group_dest_path);

  result_t res_files_list = get_files_and_dirs_list(group_path);
  files_list_t *files_list = PROPAGATE(res_files_list);

  for (u64 i = 0; i < files_list->files_count; i++) {
    if (strcmp(files_list->files[i], ".shoggoth") == 0) {
      char dir_path[FILE_PATH_SIZE];
      sprintf(dir_path, "%s/%s", group_path, files_list->files[i]);

      char dir_dest_path[FILE_PATH_SIZE];
      sprintf(dir_dest_path, "%s/%s", group_dest_path, files_list->files[i]);

      copy_dir(dir_path, dir_dest_path);

      continue;
    }

    char repo_path[FILE_PATH_SIZE];
    sprintf(repo_path, "%s/%s", group_path, files_list->files[i]);

    char repo_dest_path[FILE_PATH_SIZE];
    sprintf(repo_dest_path, "%s/%s", group_dest_path, files_list->files[i]);

    create_dir(repo_dest_path);

    char repo_dotgit_path[FILE_PATH_SIZE];
    sprintf(repo_dotgit_path, "%s/.git", repo_path);

    char repo_dest_dotgit_path[FILE_PATH_SIZE];
    sprintf(repo_dest_dotgit_path, "%s/.git", repo_dest_path);

    copy_dir(repo_dotgit_path, repo_dest_dotgit_path);

    copy_git_repo(ctx, repo_path, repo_dest_path);
  }

  free_files_list(files_list);

  return OK(NULL);
}

result_t copy_profile(client_ctx_t *ctx, char *source_path,
                      char *destination_path) {
  create_dir(destination_path);

  char metadata_path[FILE_PATH_SIZE];
  sprintf(metadata_path, "%s/.shoggoth", source_path);

  char metadata_dest_path[FILE_PATH_SIZE];
  sprintf(metadata_dest_path, "%s/.shoggoth", destination_path);
  result_t res_copy = copy_dir(metadata_path, metadata_dest_path);
  PROPAGATE(res_copy);

  res_copy = copy_group(ctx, "code", source_path, destination_path);
  PROPAGATE(res_copy);

  res_copy = copy_group(ctx, "models", source_path, destination_path);
  PROPAGATE(res_copy);

  res_copy = copy_group(ctx, "datasets", source_path, destination_path);
  PROPAGATE(res_copy);

  res_copy = copy_group(ctx, "papers", source_path, destination_path);
  PROPAGATE(res_copy);

  return OK(NULL);
}

/****
 * takes the tarball archive (located in runtime client tmp directory) of the
 * profile in the current working directory and uploads it to a node
 *
 ****/
result_t client_publish_profile(client_ctx_t *ctx) {
  LOG(INFO, "Publishing profile ...");

  char *profile_path = ".";

  char metadata_path[FILE_PATH_SIZE];
  sprintf(metadata_path, "%s/.shoggoth", profile_path);

  if (!dir_exists(metadata_path)) {
    return ERR("the directory is not a valid Shoggoth profile: no .shoggoth");
  }

  char tmp_dir_path[FILE_PATH_SIZE];
  utils_get_client_tmp_path(ctx, tmp_dir_path);
  strcat(tmp_dir_path, "/profile");

  result_t res_copy = copy_profile(ctx, profile_path, tmp_dir_path);
  PROPAGATE(res_copy);

  result_t res_gen = generate_group_signatures(ctx);
  PROPAGATE(res_gen);

  result_t valid = validate_profile(tmp_dir_path);
  PROPAGATE(valid);

  char tmp_tarball_path[FILE_PATH_SIZE];
  utils_get_client_tmp_path(ctx, tmp_tarball_path);
  strcat(tmp_tarball_path, "/profile.tar");

  if (file_exists(tmp_tarball_path)) {
    result_t res_delete = delete_file(tmp_tarball_path);
    PROPAGATE(res_delete);
  }

  result_t res_tarball = utils_create_tarball(tmp_dir_path, tmp_tarball_path);
  PROPAGATE(res_tarball);

  result_t res_delete = delete_dir(tmp_dir_path);
  PROPAGATE(res_delete);

  result_t res_file_mapping = map_file(tmp_tarball_path);
  file_mapping_t *file_mapping = PROPAGATE(res_file_mapping);

  char tmp_path[FILE_PATH_SIZE];
  utils_get_client_tmp_path(ctx, tmp_path);

  result_t res_file_hash = utils_hash_tarball(tmp_path, tmp_tarball_path);
  char *file_hash = PROPAGATE(res_file_hash);

  char delegated_node[FILE_PATH_SIZE];
  result_t res = client_delegate_node(ctx, delegated_node);
  PROPAGATE(res);

  char req_url[FILE_PATH_SIZE];
  sprintf(req_url, "%s%s", delegated_node, "/api/publish");

  sonic_request_t *req = sonic_new_request(METHOD_GET, req_url);

  u64 ts = get_timestamp_ms();

  char timestamp[50];
  sprintf(timestamp, U64_FORMAT_SPECIFIER, ts);

  fingerprint_t *fingerprint = calloc(1, sizeof(fingerprint_t));
  fingerprint->shoggoth_id = strdup(ctx->manifest->shoggoth_id);

  fingerprint->public_key = strdup(ctx->manifest->public_key);
  fingerprint->hash = strdup(file_hash),
  fingerprint->timestamp = strdup(timestamp);

  result_t res_fingerprint_json = json_fingerprint_to_json(*fingerprint);
  json_t *fingerprint_json = PROPAGATE(res_fingerprint_json);

  char *fingerprint_str = json_to_string(fingerprint_json);
  free_json(fingerprint_json);
  free_fingerprint(fingerprint);

  char private_key_path[FILE_PATH_SIZE];
  utils_get_client_runtime_path(ctx, private_key_path);
  strcat(private_key_path, "/keys/private.txt");

  result_t res_fingerprint_signature =
      openssl_sign_data(private_key_path, fingerprint_str);
  char *fingerprint_signature = PROPAGATE(res_fingerprint_signature);

  sonic_add_header(req, "shoggoth-id", ctx->manifest->shoggoth_id);
  sonic_add_header(req, "fingerprint", fingerprint_str);
  sonic_add_header(req, "signature", fingerprint_signature);

  u64 upload_size = (u64)file_mapping->info.st_size;
  u64 chunk_size_limit = 100000; // 100 KB

  u64 chunk_count = 0;

  if (upload_size > chunk_size_limit) {
    chunk_count = upload_size / chunk_size_limit;
  } else {
    chunk_count = 1;
  }

  if (upload_size > chunk_size_limit && upload_size % chunk_size_limit > 0) {
    chunk_count++;
  }

  if (upload_size < chunk_size_limit) {
    chunk_count = 1;
  }

  char upload_size_str[128];
  sprintf(upload_size_str, U64_FORMAT_SPECIFIER, upload_size);
  sonic_add_header(req, "upload-size", upload_size_str);

  char chunk_size_limit_str[128];
  sprintf(chunk_size_limit_str, U64_FORMAT_SPECIFIER, chunk_size_limit);
  sonic_add_header(req, "chunk-size-limit", chunk_size_limit_str);

  char chunk_count_str[128];
  sprintf(chunk_count_str, U64_FORMAT_SPECIFIER, chunk_count);
  sonic_add_header(req, "chunk-count", chunk_count_str);

  sonic_response_t *resp = sonic_send_request(req);
  if (resp->failed) {
    return ERR("CLIENT PUBLISH FAILED: %s", resp->error);
  } else {
    char body[512];
    sprintf(body, "%.*s", (int)resp->response_body_size, resp->response_body);

    if (resp->status == STATUS_200) {
    } else if (resp->status == STATUS_406) {
      return ERR("NEGOTIATION FAILED: %s", body);
    } else {
      return ERR("NEGOTIATION FAILED: status was not OK: %s", body);
    }
  }

  assert(resp->response_body_size > 0);

  char *upload_id = malloc((resp->response_body_size + 1) * sizeof(char));
  memcpy(upload_id, resp->response_body, resp->response_body_size);
  upload_id[resp->response_body_size] = '\0';

  free(resp->response_body);

  sonic_free_request(req);
  sonic_free_response(resp);

  tuwi_set_cursor_style(STYLE_INVISIBLE);

  for (u64 i = 0; i < chunk_count; i++) {
    u64 chunk_id = i;
    char chunk_id_str[128];
    sprintf(chunk_id_str, U64_FORMAT_SPECIFIER, chunk_id);

    u64 chunk_size = 0;
    if (chunk_id == chunk_count - 1) {
      chunk_size = upload_size % chunk_size_limit;
    } else {
      chunk_size = chunk_size_limit;
    }

    char chunk_size_str[128];
    sprintf(chunk_size_str, U64_FORMAT_SPECIFIER, chunk_size);

    u64 current_chunk_content_index =
        ((chunk_id + 1) * chunk_size_limit) - chunk_size_limit;
    char *current_chunk_content =
        &file_mapping->content[current_chunk_content_index];

    u64 total_sent_size = ((chunk_id + 1) * chunk_size_limit);

    total_sent_size =
        total_sent_size <= upload_size ? total_sent_size : upload_size;

    f64 sent_percentage =
        (f64)((f64)total_sent_size / (f64)upload_size) * 100.0;

    tuwi_clear_current_line();
    tuwi_flush_terminal();
    printf("UPLOADING CHUNK: " U64_FORMAT_SPECIFIER "/" U64_FORMAT_SPECIFIER
           " - " U64_FORMAT_SPECIFIER " bytes - " U64_FORMAT_SPECIFIER
           "/" U64_FORMAT_SPECIFIER " bytes - %.1f%%",
           chunk_id, chunk_count, chunk_size, total_sent_size, upload_size,
           sent_percentage);
    tuwi_flush_terminal();

    char chunk_req_url[FILE_PATH_SIZE];
    sprintf(chunk_req_url, "%s%s", delegated_node, "/api/publish_chunk");

    sonic_request_t *chunk_req = sonic_new_request(METHOD_GET, chunk_req_url);

    sonic_set_body(chunk_req, current_chunk_content, chunk_size);

    sonic_add_header(chunk_req, "chunk-id", chunk_id_str);
    sonic_add_header(chunk_req, "upload-id", upload_id);
    sonic_add_header(chunk_req, "chunk-size", chunk_size_str);

    sonic_response_t *chunk_resp = sonic_send_request(chunk_req);
    if (chunk_resp->failed) {
      tuwi_set_cursor_style(STYLE_NORMAL);
      printf("\n");
      return ERR("UPLOAD CHUNK %lu FAILED: %s", chunk_id, chunk_resp->error);
    } else if (chunk_resp->status != STATUS_200) {
      tuwi_set_cursor_style(STYLE_NORMAL);
      printf("\n");

      char body[512];
      sprintf(body, "%.*s", (int)chunk_resp->response_body_size,
              chunk_resp->response_body);

      return ERR("UPLOAD CHUNK %lu FAILED: status was not OK: %s", chunk_id,
                 body);
    }

    sonic_free_request(chunk_req);
    sonic_free_response(chunk_resp);
  }

  tuwi_set_cursor_style(STYLE_NORMAL);
  printf("\n");

  char finish_req_url[FILE_PATH_SIZE];
  sprintf(finish_req_url, "%s%s", delegated_node, "/api/publish_finish");

  sonic_request_t *finish_req = sonic_new_request(METHOD_GET, finish_req_url);

  sonic_add_header(finish_req, "upload-id", upload_id);

  sonic_response_t *finish_resp = sonic_send_request(finish_req);
  if (finish_resp->failed) {
    return ERR("UPLOAD FINISH FAILED: %s", finish_resp->error);
  } else {
    assert(finish_resp->response_body_size > 0);

    char body[512];
    sprintf(body, "%.*s", (int)finish_resp->response_body_size,
            finish_resp->response_body);

    if (finish_resp->status == STATUS_200) {
      LOG(INFO, "PUBLISH COMPLETE!");
      printf("You can view your profile at %s/explorer/profile/%s \n",
             delegated_node, ctx->manifest->shoggoth_id);
    } else if (finish_resp->status == STATUS_202) {
      LOG(INFO, "UPDATE COMPLETE!");
      printf("You can view your profile at %s/explorer/profile/%s \n",
             delegated_node, ctx->manifest->shoggoth_id);
    } else if (finish_resp->status == STATUS_406) {
      return ERR("UPLOAD FINISH FAILED: %s", body);
    } else {
      return ERR("PUBLISH FINISH FAILED: status was not OK: %s", body);
    }
  }

  if (finish_resp->response_body_size > 0) {
    free(finish_resp->response_body);
  }

  sonic_free_request(finish_req);
  sonic_free_response(finish_resp);

  free(upload_id);
  result_t res_delete_tarball = delete_file(tmp_tarball_path);
  PROPAGATE(res_delete_tarball);

  unmap_file(file_mapping);
  free(file_hash);
  free(fingerprint_str);
  free(fingerprint_signature);

  return OK(NULL);
}
