/**** clone.c ****
 *
 *  Copyright (c) 2023 Shoggoth Systems
 *
 * Part of the Shoggoth project, under the MIT License.
 * See LICENCE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#include "../json/json.h"

#include <stdlib.h>

typedef enum {
  PROFILE,
  GROUP,
  RESOURCE,
} request_target_t;

typedef struct {
  char *shoggoth_id;
  char *group;
  char *resource;

  request_target_t target;
} request_id_t;

void free_request_id(request_id_t id) {
  free(id.shoggoth_id);
  free(id.group);
  free(id.resource);
}

request_id_t parse_id(char *input) {
  request_id_t result;
  result.shoggoth_id = NULL;
  result.group = NULL;
  result.resource = NULL;

  char *copy =
      strdup(input); // Create a copy to avoid modifying the original string
  char *token = strtok(copy, "/");

  if (token) {
    result.shoggoth_id = strdup(token);
    token = strtok(NULL, "/");
  }

  if (token) {
    result.group = strdup(token);
    token = strtok(NULL, "/");
  }

  if (token) {
    result.resource = strdup(token);
  }

  free(copy); // Free the copy of the input string

  if (result.group == NULL && result.resource == NULL) {
    result.target = PROFILE;
  } else if (result.group != NULL && result.resource == NULL) {
    result.target = GROUP;
  } else if (result.group != NULL && result.resource != NULL) {
    result.target = RESOURCE;
  }

  return result;
}

void clone_response_callback(char *data, u64 size, void *user_pointer) {
  char *tmp_zip_path = (char *)user_pointer;

  result_t res = append_to_file(tmp_zip_path, data, size);
  UNWRAP(res);
}

/****
 * downloads the Shoggoth profile of shoggoth_id into the current working
 *directory
 *
 ****/
result_t client_clone(client_ctx_t *ctx, char *id_str) {
  request_id_t id = parse_id(id_str);

  if (id.target == PROFILE) {
    printf("Cloning profile %s \n", id.shoggoth_id);
  } else if (id.target == GROUP) {
    printf("Cloning group %s/%s \n", id.shoggoth_id, id.group);
  } else if (id.target == RESOURCE) {
    printf("Cloning resource %s/%s/%s \n", id.shoggoth_id, id.group,
           id.resource);
  } else {
    return ERR("unhandled clone request target");
  }

  char tmp_path[FILE_PATH_SIZE];
  utils_get_client_tmp_path(ctx, tmp_path);

  char tmp_dir_path[FILE_PATH_SIZE];
  if (id.target == PROFILE) {
    sprintf(tmp_dir_path, "%s/%s", tmp_path, id.shoggoth_id);
  } else if (id.target == GROUP) {
    sprintf(tmp_dir_path, "%s/%s.%s", tmp_path, id.shoggoth_id, id.group);
  } else if (id.target == RESOURCE) {
    sprintf(tmp_dir_path, "%s/%s.%s.%s", tmp_path, id.shoggoth_id, id.group,
            id.resource);
  }

  char tmp_zip_path[FILE_PATH_SIZE];
  if (id.target == PROFILE) {
    sprintf(tmp_zip_path, "%s/%s.tar", tmp_path, id.shoggoth_id);
  } else if (id.target == GROUP) {
    sprintf(tmp_zip_path, "%s/%s.%s.tar", tmp_path, id.shoggoth_id, id.group);
  } else if (id.target == RESOURCE) {
    sprintf(tmp_zip_path, "%s/%s.%s.%s.tar", tmp_path, id.shoggoth_id, id.group,
            id.resource);
  }

  char node_host[FILE_PATH_SIZE];
  result_t res_del = client_delegate_node(ctx, node_host);
  PROPAGATE(res_del);

  char request_url[256];
  sprintf(request_url, "%s/api/clone/%s", node_host, id_str);

  sonic_request_t *req = sonic_new_request(METHOD_GET, request_url);

  char *user_ptr = strdup(tmp_zip_path);

  sonic_request_set_response_callback(req, clone_response_callback, user_ptr);

  sonic_response_t *resp = sonic_send_request(req);

  free(user_ptr);

  if (resp->failed) {
    return ERR("CLONE FAILED: %s", sonic_get_error());
  } else {
    char body[512];
    sprintf(body, "%.*s", (int)resp->response_body_size, resp->response_body);

    if (resp->status == STATUS_200) {
    } else if (resp->status == STATUS_400) {
      return ERR("CLONE FAILED: %s", body);
    } else if (resp->status == STATUS_404) {
      return ERR("CLONE FAILED: %s", body);
    } else if (resp->status == STATUS_406) {
      return ERR("CLONE FAILED: %s", body);
    } else {
      return ERR("CLONE FAILED: status was not OK");
    }
  }

  result_t res_unzip = utils_extract_tarball(tmp_zip_path, tmp_dir_path);
  PROPAGATE(res_unzip);

  if (id.target == PROFILE) {
    result_t dir_valid = validate_profile(tmp_dir_path);
    PROPAGATE(dir_valid);
  } else if (id.target == GROUP) {
    result_t dir_valid = validate_resource_group(tmp_dir_path);
    PROPAGATE(dir_valid);
  } else if (id.target == RESOURCE) {
    result_t dir_valid = validate_git_repo(tmp_dir_path);
    PROPAGATE(dir_valid);
  }

  char *fingerprint_str =
      sonic_get_header_value(resp->headers, resp->headers_count, "fingerprint");
  if (fingerprint_str == NULL) {
    return ERR("no fingerprint header was found in remote clone response");
  }

  char *fingerprint_signature =
      sonic_get_header_value(resp->headers, resp->headers_count, "signature");
  if (fingerprint_signature == NULL) {
    return ERR("no fingerprint header was found in remote clone response");
  }

  char *fingerprint_hash = NULL;
  char *fingerprint_public_key = NULL;

  if (id.target == PROFILE) {
    result_t res_fingerprint = json_string_to_fingerprint(fingerprint_str);
    fingerprint_t *fingerprint = PROPAGATE(res_fingerprint);

    fingerprint_hash = strdup(fingerprint->hash);
    fingerprint_public_key = strdup(fingerprint->public_key);

    free_fingerprint(fingerprint);
  } else if (id.target == GROUP) {
    result_t res_fingerprint =
        json_string_to_group_fingerprint(fingerprint_str);
    group_fingerprint_t *fingerprint = PROPAGATE(res_fingerprint);

    fingerprint_hash = strdup(fingerprint->hash);
    fingerprint_public_key = strdup(fingerprint->public_key);

    free_group_fingerprint(fingerprint);
  } else if (id.target == RESOURCE) {
    result_t res_fingerprint =
        json_string_to_resource_fingerprint(fingerprint_str);
    resource_fingerprint_t *fingerprint = PROPAGATE(res_fingerprint);

    fingerprint_hash = strdup(fingerprint->hash);
    fingerprint_public_key = strdup(fingerprint->public_key);

    free_resource_fingerprint(fingerprint);
  }

  result_t valid = validate_authorization(
      tmp_path, tmp_zip_path, fingerprint_hash, fingerprint_public_key,
      fingerprint_str, fingerprint_signature);

  free(fingerprint_hash);
  free(fingerprint_public_key);

  PROPAGATE(valid);

  char final_dir_path[256];
  if (id.target == PROFILE) {
    sprintf(final_dir_path, "./%s", id.shoggoth_id);
  } else if (id.target == GROUP) {
    sprintf(final_dir_path, "./%s.%s", id.shoggoth_id, id.group);
  } else if (id.target == RESOURCE) {
    sprintf(final_dir_path, "./%s", id.resource);
  }

  res_unzip = utils_extract_tarball(tmp_zip_path, final_dir_path);
  PROPAGATE(res_unzip);

  result_t res_delete = delete_file(tmp_zip_path);
  PROPAGATE(res_delete);

  res_delete = delete_dir(tmp_dir_path);
  PROPAGATE(res_delete);

  LOG(INFO, "Cloned successfully");

  free_request_id(id);

  free(resp->response_body);

  sonic_free_request(req);
  sonic_free_response(resp);

  return OK(NULL);
}
