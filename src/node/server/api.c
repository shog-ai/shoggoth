/**** api.c ****
 *
 *  Copyright (c) 2023 Shoggoth Systems
 *
 * Part of the Shoggoth project, under the MIT License.
 * See LICENCE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#include "../../json/json.h"
#include "../db/db.h"
#include "../manifest/manifest.h"
#include "pin.h"

#include <assert.h>
#include <stdlib.h>

node_ctx_t *api_ctx = NULL;

typedef enum {
  PROFILE,
  GROUP,
  RESOURCE,
} request_target_t;

void respond_error(sonic_server_request_t *req, char *error_message) {
  sonic_server_response_t *resp =
      sonic_new_response(STATUS_406, MIME_TEXT_PLAIN);

  if (error_message != NULL) {
    sonic_response_set_body(resp, error_message, strlen(error_message));
  }

  sonic_send_response(req, resp);
  sonic_free_server_response(resp);
}

void api_download_route(sonic_server_request_t *req) {
  char *shoggoth_id = sonic_get_path_segment(req, "shoggoth_id");
  char *group_name = sonic_get_path_segment(req, "group_name");
  char *resource_name = sonic_get_path_segment(req, "resource_name");

  resource_name[strlen(resource_name) - 4] = '\0';

  LOG(INFO, "RES: %s", resource_name);

  if (strlen(shoggoth_id) != 36) {
    respond_error(req, "invalid Shoggoth ID");

    return;
  }

  char runtime_path[FILE_PATH_SIZE];
  utils_get_node_runtime_path(api_ctx, runtime_path);

  char pin_dir_path[FILE_PATH_SIZE];
  sprintf(pin_dir_path, "%s/pins/%s", runtime_path, shoggoth_id);

  char target_dir_path[FILE_PATH_SIZE];
  sprintf(target_dir_path, "%s/pins/%s/%s/%s", runtime_path, shoggoth_id,
          group_name, resource_name);

  char target_tarball_path[FILE_PATH_SIZE];
  sprintf(target_tarball_path, "%s/tmp/%s.%s.%s.tar", runtime_path, shoggoth_id,
          group_name, resource_name);

  char target_tmp_path[FILE_PATH_SIZE];
  sprintf(target_tmp_path, "%s/tmp/%s.%s.%s", runtime_path, shoggoth_id,
          group_name, resource_name);

  // FIXME: run checks on the value of group_name and resource_name to ensure it
  // does not contain unsafe characters like a dot "." or slash "/"

  if (!file_exists(pin_dir_path)) {
    result_t res_peers_with_pin = db_get_peers_with_pin(api_ctx, shoggoth_id);
    SERVER_ERR(res_peers_with_pin);
    char *peers_with_pin = VALUE(res_peers_with_pin);

    result_t res_peers = json_string_to_dht(peers_with_pin);
    SERVER_ERR(res_peers);
    dht_t *peers = VALUE(res_peers);

    free(peers_with_pin);

    if (peers->items_count > 0) {
      char location[256];
      sprintf(location, "%s/api/download/%s/%s/%s.tar", peers->items[0]->host,
              shoggoth_id, group_name, resource_name);

      sonic_server_response_t *resp =
          sonic_new_response(STATUS_302, MIME_TEXT_PLAIN);
      sonic_response_add_header(resp, "Location", location);

      sonic_send_response(req, resp);
      sonic_free_server_response(resp);
    } else {
      sonic_server_response_t *resp =
          sonic_new_response(STATUS_406, MIME_TEXT_PLAIN);
      sonic_send_response(req, resp);
      sonic_free_server_response(resp);
    }

    free_dht(peers);
  } else {
    if (!dir_exists(target_dir_path)) {
      sonic_server_response_t *resp =
          sonic_new_response(STATUS_404, MIME_TEXT_HTML);

      char *body = "the resource was not found";

      sonic_response_set_body(resp, body, (u64)strlen(body));

      sonic_send_response(req, resp);

      sonic_free_server_response(resp);

      return;
    }

    result_t res_target_tmp_lock =
        acquire_file_lock(target_tmp_path, 1000, 20000);
    SERVER_ERR(res_target_tmp_lock);
    file_lock_t *target_tmp_lock = VALUE(res_target_tmp_lock);

    result_t res_copy = copy_dir(target_dir_path, target_tmp_path);
    SERVER_ERR(res_copy);

    result_t res_tarball =
        utils_create_tarball(target_tmp_path, target_tarball_path);
    SERVER_ERR(res_tarball);

    result_t res_delete = delete_dir(target_tmp_path);
    SERVER_ERR(res_delete);

    result_t res_file_mapping = map_file(target_tarball_path);
    SERVER_ERR(res_file_mapping);
    file_mapping_t *file_mapping = VALUE(res_file_mapping);

    sonic_server_response_t *resp =
        sonic_new_response(STATUS_200, MIME_APPLICATION_OCTET_STREAM);
    sonic_response_set_body(resp, file_mapping->content,
                            (u64)file_mapping->info.st_size);

    char fingerprint_path[FILE_PATH_SIZE];
    sprintf(fingerprint_path,
            "%s/pins/%s/%s/.shoggoth/fingerprints/%s/fingerprint.json",
            runtime_path, shoggoth_id, group_name, resource_name);

    result_t res_fingerprint_str = read_file_to_string(fingerprint_path);
    SERVER_ERR(res_fingerprint_str);
    char *fingerprint_str = VALUE(res_fingerprint_str);

    sonic_response_add_header(resp, "fingerprint", fingerprint_str);

    char signature_path[FILE_PATH_SIZE];
    sprintf(signature_path,
            "%s/pins/%s/%s/.shoggoth/fingerprints/%s/signature.txt",
            runtime_path, shoggoth_id, group_name, resource_name);

    result_t res_signature_str = read_file_to_string(signature_path);
    SERVER_ERR(res_signature_str);
    char *signature_str = VALUE(res_signature_str);

    sonic_response_add_header(resp, "signature", signature_str);

    sonic_send_response(req, resp);

    unmap_file(file_mapping);
    free(fingerprint_str);
    free(signature_str);
    sonic_free_server_response(resp);

    res_delete = delete_file(target_tarball_path);
    SERVER_ERR(res_delete);

    release_file_lock(target_tmp_lock);
  }
}

void api_clone_route(sonic_server_request_t *req) {
  char *shoggoth_id = sonic_get_path_segment(req, "shoggoth_id");
  char *group_name = sonic_get_path_segment(req, "group_name");
  char *resource_name = sonic_get_path_segment(req, "resource_name");

  request_target_t target = PROFILE;

  if (group_name == NULL && resource_name == NULL) {
    target = PROFILE;
  } else if (group_name != NULL && resource_name == NULL) {
    target = GROUP;
  } else if (group_name != NULL && resource_name != NULL) {
    target = RESOURCE;
  }

  if (strlen(shoggoth_id) != 36) {
    respond_error(req, "invalid Shoggoth ID");

    return;
  }

  if (target == GROUP || target == RESOURCE) {
    if (strcmp(group_name, "code") != 0 && strcmp(group_name, "models") != 0 &&
        strcmp(group_name, "datasets") != 0 &&
        strcmp(group_name, "papers") != 0) {
      respond_error(req, "invalid group name");

      return;
    }
  }

  char runtime_path[FILE_PATH_SIZE];
  utils_get_node_runtime_path(api_ctx, runtime_path);

  char pin_dir_path[FILE_PATH_SIZE];
  sprintf(pin_dir_path, "%s/pins/%s", runtime_path, shoggoth_id);

  char target_dir_path[FILE_PATH_SIZE];
  if (target == PROFILE) {
    sprintf(target_dir_path, "%s/pins/%s", runtime_path, shoggoth_id);
  } else if (target == GROUP) {
    sprintf(target_dir_path, "%s/pins/%s/%s", runtime_path, shoggoth_id,
            group_name);
  } else if (target == RESOURCE) {
    sprintf(target_dir_path, "%s/pins/%s/%s/%s", runtime_path, shoggoth_id,
            group_name, resource_name);
  }

  char target_tarball_path[FILE_PATH_SIZE];
  if (target == PROFILE) {
    sprintf(target_tarball_path, "%s/tmp/%s.tar", runtime_path, shoggoth_id);
  } else if (target == GROUP) {
    sprintf(target_tarball_path, "%s/tmp/%s.%s.tar", runtime_path, shoggoth_id,
            group_name);
  } else if (target == RESOURCE) {
    sprintf(target_tarball_path, "%s/tmp/%s.%s.%s.tar", runtime_path,
            shoggoth_id, group_name, resource_name);
  }

  char target_tmp_path[FILE_PATH_SIZE];
  if (target == PROFILE) {
    sprintf(target_tmp_path, "%s/tmp/%s", runtime_path, shoggoth_id);
  } else if (target == GROUP) {
    sprintf(target_tmp_path, "%s/tmp/%s.%s", runtime_path, shoggoth_id,
            group_name);
  } else if (target == RESOURCE) {
    sprintf(target_tmp_path, "%s/tmp/%s.%s.%s", runtime_path, shoggoth_id,
            group_name, resource_name);
  }

  // FIXME: run checks on the value of group_name and resource_name to ensure it
  // does not contain unsafe characters like a dot "." or slash "/"

  if (!file_exists(pin_dir_path)) {
    result_t res_peers_with_pin = db_get_peers_with_pin(api_ctx, shoggoth_id);
    SERVER_ERR(res_peers_with_pin);
    char *peers_with_pin = VALUE(res_peers_with_pin);

    result_t res_peers = json_string_to_dht(peers_with_pin);
    SERVER_ERR(res_peers);
    dht_t *peers = VALUE(res_peers);

    free(peers_with_pin);

    if (peers->items_count > 0) {
      char location[256];
      if (target == PROFILE) {
        sprintf(location, "%s/api/clone/%s", peers->items[0]->host,
                shoggoth_id);
      } else if (target == GROUP) {
        sprintf(location, "%s/api/clone/%s/%s", peers->items[0]->host,
                shoggoth_id, group_name);
      } else if (target == RESOURCE) {
        sprintf(location, "%s/api/clone/%s/%s/%s", peers->items[0]->host,
                shoggoth_id, group_name, resource_name);
      }

      // LOG(INFO, "Location: %s", location);

      sonic_server_response_t *resp =
          sonic_new_response(STATUS_302, MIME_TEXT_PLAIN);
      sonic_response_add_header(resp, "Location", location);

      sonic_send_response(req, resp);
      sonic_free_server_response(resp);
    } else {
      sonic_server_response_t *resp =
          sonic_new_response(STATUS_406, MIME_TEXT_PLAIN);
      sonic_send_response(req, resp);
      sonic_free_server_response(resp);
    }

    free_dht(peers);
  } else {
    if (target == RESOURCE) {
      if (!dir_exists(target_dir_path)) {
        sonic_server_response_t *resp =
            sonic_new_response(STATUS_404, MIME_TEXT_HTML);

        char *body = "the resource was not found";

        sonic_response_set_body(resp, body, (u64)strlen(body));

        sonic_send_response(req, resp);

        sonic_free_server_response(resp);

        return;
      }
    }

    result_t res_target_tmp_lock =
        acquire_file_lock(target_tmp_path, 1000, 20000);
    SERVER_ERR(res_target_tmp_lock);
    file_lock_t *target_tmp_lock = VALUE(res_target_tmp_lock);

    result_t res_copy = copy_dir(target_dir_path, target_tmp_path);
    SERVER_ERR(res_copy);

    if (target == PROFILE) {
      char fingerprint_tmp_path[FILE_PATH_SIZE];
      sprintf(fingerprint_tmp_path, "%s/.shoggoth/fingerprint.json",
              target_tmp_path);

      char signature_tmp_path[FILE_PATH_SIZE];
      sprintf(signature_tmp_path, "%s/.shoggoth/signature.txt",
              target_tmp_path);

      result_t res_delete = delete_file(fingerprint_tmp_path);
      SERVER_ERR(res_delete);

      res_delete = delete_file(signature_tmp_path);
      SERVER_ERR(res_delete);
    } else if (target == GROUP) {
      char fingerprint_path[FILE_PATH_SIZE];
      sprintf(fingerprint_path, "%s/.shoggoth/fingerprint.json",
              target_tmp_path);

      result_t res_delete = delete_file(fingerprint_path);
      SERVER_ERR(res_delete);

      char signature_path[FILE_PATH_SIZE];
      sprintf(signature_path, "%s/.shoggoth/signature.txt", target_tmp_path);

      res_delete = delete_file(signature_path);
      SERVER_ERR(res_delete);
    }

    result_t res_tarball =
        utils_create_tarball(target_tmp_path, target_tarball_path);
    SERVER_ERR(res_tarball);

    result_t res_delete = delete_dir(target_tmp_path);
    SERVER_ERR(res_delete);

    result_t res_file_mapping = map_file(target_tarball_path);
    SERVER_ERR(res_file_mapping);
    file_mapping_t *file_mapping = VALUE(res_file_mapping);

    sonic_server_response_t *resp =
        sonic_new_response(STATUS_200, MIME_APPLICATION_OCTET_STREAM);
    sonic_response_set_body(resp, file_mapping->content,
                            (u64)file_mapping->info.st_size);

    char fingerprint_path[FILE_PATH_SIZE];
    if (target == PROFILE) {
      sprintf(fingerprint_path, "%s/pins/%s/.shoggoth/fingerprint.json",
              runtime_path, shoggoth_id);
    } else if (target == GROUP) {
      sprintf(fingerprint_path, "%s/pins/%s/%s/.shoggoth/fingerprint.json",
              runtime_path, shoggoth_id, group_name);
    } else if (target == RESOURCE) {
      sprintf(fingerprint_path,
              "%s/pins/%s/%s/.shoggoth/fingerprints/%s/fingerprint.json",
              runtime_path, shoggoth_id, group_name, resource_name);
    }

    result_t res_fingerprint_str = read_file_to_string(fingerprint_path);
    SERVER_ERR(res_fingerprint_str);
    char *fingerprint_str = VALUE(res_fingerprint_str);

    sonic_response_add_header(resp, "fingerprint", fingerprint_str);

    char signature_path[FILE_PATH_SIZE];
    if (target == PROFILE) {
      sprintf(signature_path, "%s/pins/%s/.shoggoth/signature.txt",
              runtime_path, shoggoth_id);
    } else if (target == GROUP) {
      sprintf(signature_path, "%s/pins/%s/%s/.shoggoth/signature.txt",
              runtime_path, shoggoth_id, group_name);
    } else if (target == RESOURCE) {
      sprintf(signature_path,
              "%s/pins/%s/%s/.shoggoth/fingerprints/%s/signature.txt",
              runtime_path, shoggoth_id, group_name, resource_name);
    }

    result_t res_signature_str = read_file_to_string(signature_path);
    SERVER_ERR(res_signature_str);
    char *signature_str = VALUE(res_signature_str);

    sonic_response_add_header(resp, "signature", signature_str);

    sonic_send_response(req, resp);

    unmap_file(file_mapping);
    free(fingerprint_str);
    free(signature_str);
    sonic_free_server_response(resp);

    res_delete = delete_file(target_tarball_path);
    SERVER_ERR(res_delete);

    release_file_lock(target_tmp_lock);
  }
}

void free_upload_info(upload_info_t *info) {
  free(info->shoggoth_id);

  free(info);
}

void api_negotiate_publish_route(sonic_server_request_t *req) {
  // FIXME: validate the shoggoth_id

  if (!api_ctx->config->pins.allow_publish) {
    respond_error(req, "this node has disabled publishing profiles");
    return;
  }

  char *upload_id = utils_gen_uuid();

  char *shoggoth_id =
      sonic_get_header_value(req->headers, req->headers_count, "shoggoth-id");
  if (shoggoth_id == NULL) {
    respond_error(req, "no shoggoth-id header in publish request");
    return;
  }

  char *chunk_count_str =
      sonic_get_header_value(req->headers, req->headers_count, "chunk-count");
  if (chunk_count_str == NULL) {
    respond_error(req, "no chunk-count header in publish request");
    return;
  }

  char *chunk_size_limit_str = sonic_get_header_value(
      req->headers, req->headers_count, "chunk-size-limit");
  if (chunk_size_limit_str == NULL) {
    respond_error(req, "no chunk-size-limit header in publish request");
    return;
  }

  u64 received_chunk_size_limit = (u64)atoll(chunk_size_limit_str);

  u64 chunk_size_limit = 100000;

  if (received_chunk_size_limit != chunk_size_limit) {
    respond_error(req, "received_chunk_size_limit != chunk_size_limit");
    return;
  }

  char *upload_size_str =
      sonic_get_header_value(req->headers, req->headers_count, "upload-size");
  if (upload_size_str == NULL) {
    respond_error(req, "no upload-size header in publish request \n");
    return;
  }

  u64 upload_size = (u64)atoll(upload_size_str);

  char *fingerprint_str =
      sonic_get_header_value(req->headers, req->headers_count, "fingerprint");
  if (fingerprint_str == NULL) {
    respond_error(req, "no fingerprint header in publish request \n");
    return;
  }

  char *received_signature =
      sonic_get_header_value(req->headers, req->headers_count, "signature");
  if (received_signature == NULL) {
    respond_error(req, "no signature header in publish request \n");
    return;
  }

  char pins_path[256];
  utils_get_node_runtime_path(api_ctx, pins_path);
  strcat(pins_path, "/pins");

  u64 profile_size_limit = (u64)(api_ctx->config->storage.max_profile_size *
                                 1000000.0); // convert megabytes to bytes

  result_t res_total_pins_size = get_dir_size(pins_path);
  SERVER_ERR(res_total_pins_size);
  u64 total_pins_size = VALUE_U64(res_total_pins_size);

  u64 total_pins_limit = (u64)(api_ctx->config->storage.limit *
                               1000000000.0); // convert gigabytes to bytes

  if (upload_size > profile_size_limit) {
    sonic_server_response_t *resp =
        sonic_new_response(STATUS_406, MIME_TEXT_PLAIN);

    char body[512];
    sprintf(body,
            "Sorry, your profile is larger than the maximum profile size for "
            "this node.\nLimit: " U64_FORMAT_SPECIFIER
            " bytes\nYour profile: " U64_FORMAT_SPECIFIER " bytes",
            profile_size_limit, upload_size);

    sonic_response_set_body(resp, strdup(body), strlen(body));

    sonic_send_response(req, resp);

    sonic_free_server_response(resp);

    return;
  } else if (upload_size + total_pins_size > total_pins_limit) {
    sonic_server_response_t *resp =
        sonic_new_response(STATUS_406, MIME_TEXT_PLAIN);

    char body[512];
    sprintf(body,
            "Sorry, the node storage is full. consider reducing the size of "
            "your profile\n"
            "\nYour profile: " U64_FORMAT_SPECIFIER " bytes",
            upload_size);

    sonic_response_set_body(resp, strdup(body), strlen(body));

    sonic_send_response(req, resp);

    sonic_free_server_response(resp);

    return;
  }

  u64 five_minutes = 300000; // 5 miinutes in milliseconds
  u64 now = get_timestamp_ms();

  result_t res_fingerprint = json_string_to_fingerprint(fingerprint_str);
  SERVER_ERR(res_fingerprint);
  fingerprint_t *fingerprint = VALUE(res_fingerprint);

  u64 new_ts = strtoull(fingerprint->timestamp, NULL, 10);

  if (new_ts > (now + five_minutes)) {
    sonic_server_response_t *resp =
        sonic_new_response(STATUS_406, MIME_TEXT_PLAIN);

    char *body = "the fingerprint timestamp is set in the future. are you a "
                 "time traveler?";
    sonic_response_set_body(resp, body, strlen(body));

    sonic_send_response(req, resp);

    sonic_free_server_response(resp);
  } else if (new_ts < (now - five_minutes)) {
    sonic_server_response_t *resp =
        sonic_new_response(STATUS_406, MIME_TEXT_PLAIN);

    char *body = "the fingerprint timestamp is set in the past. are you a "
                 "time traveler?";
    sonic_response_set_body(resp, body, strlen(body));

    sonic_send_response(req, resp);

    sonic_free_server_response(resp);
  }

  char final_dir_path[256];
  utils_get_node_runtime_path(api_ctx, final_dir_path);
  strcat(final_dir_path, "/pins/");
  strcat(final_dir_path, shoggoth_id);

  char metadata_dir[FILE_PATH_SIZE];
  sprintf(metadata_dir, "%s/.shoggoth", final_dir_path);

  char fingerprint_path[FILE_PATH_SIZE];
  sprintf(fingerprint_path, "%s/fingerprint.json", metadata_dir);

  bool is_update = false;
  if (dir_exists(final_dir_path)) {
    is_update = true;
  }

  bool should_update = false;
  if (is_update) {
    result_t res_old_fingerprint_str = read_file_to_string(fingerprint_path);
    SERVER_ERR(res_old_fingerprint_str);
    char *old_fingerprint_str = VALUE(res_old_fingerprint_str);

    result_t res_old_fingerprint =
        json_string_to_fingerprint(old_fingerprint_str);
    SERVER_ERR(res_old_fingerprint);
    fingerprint_t *old_fingerprint = VALUE(res_old_fingerprint);

    u64 old_ts = strtoull(old_fingerprint->timestamp, NULL, 10);
    u64 new_ts = strtoull(fingerprint->timestamp, NULL, 10);

    if (strcmp(old_fingerprint->hash, fingerprint->hash) != 0 &&
        new_ts > old_ts) {
      should_update = true;
    }

    free(old_fingerprint_str);
    free_fingerprint(old_fingerprint);
  }

  free_fingerprint(fingerprint);

  if (!is_update || (is_update && should_update)) {
    char upload_info_str[256];
    sprintf(upload_info_str,
            "{\"shoggoth_id\": \"%s\", \"chunk_count\": \"%s\", "
            "\"chunk_size_limit\": \"%s\", \"upload_size\": \"%s\"}",
            shoggoth_id, chunk_count_str, chunk_size_limit_str,
            upload_size_str);

    char tmp_path[FILE_PATH_SIZE];
    utils_get_node_tmp_path(api_ctx, tmp_path);

    char tmp_upload_path[FILE_PATH_SIZE];
    sprintf(tmp_upload_path, "%s/%s", tmp_path, upload_id);
    create_dir(tmp_upload_path);

    char tmp_signature_path[FILE_PATH_SIZE];
    sprintf(tmp_signature_path, "%s/signature.txt", tmp_upload_path);
    result_t res_write = write_to_file(tmp_signature_path, received_signature,
                                       strlen(received_signature));
    SERVER_ERR(res_write);

    char tmp_fingerprint_path[FILE_PATH_SIZE];
    sprintf(tmp_fingerprint_path, "%s/fingerprint.json", tmp_upload_path);
    res_write = write_to_file(tmp_fingerprint_path, fingerprint_str,
                              strlen(fingerprint_str));
    SERVER_ERR(res_write);

    char tmp_upload_info_path[FILE_PATH_SIZE];
    sprintf(tmp_upload_info_path, "%s/upload_info.json", tmp_upload_path);
    res_write = write_to_file(tmp_upload_info_path, upload_info_str,
                              strlen(upload_info_str));
    SERVER_ERR(res_write);

    char tmp_chunks_path[FILE_PATH_SIZE];
    sprintf(tmp_chunks_path, "%s/chunks", tmp_upload_path);
    create_dir(tmp_chunks_path);

    sonic_server_response_t *resp =
        sonic_new_response(STATUS_200, MIME_TEXT_PLAIN);

    char *body = upload_id;

    sonic_response_set_body(resp, body, strlen(body));

    sonic_send_response(req, resp);

    free(upload_id);

    sonic_free_server_response(resp);
  } else {
    sonic_server_response_t *resp =
        sonic_new_response(STATUS_406, MIME_TEXT_PLAIN);

    char body[512];
    sprintf(
        body,
        "Your profile has already been published and no changes were detected");

    sonic_response_set_body(resp, body, strlen(body));

    sonic_send_response(req, resp);

    free(upload_id);

    sonic_free_server_response(resp);
  }
}

void api_publish_finish_route(sonic_server_request_t *req) {
  // FIXME: validate the upload_id

  if (!api_ctx->config->pins.allow_publish) {
    respond_error(req, "this node has disabled publishing profiles");
    return;
  }

  char *upload_id =
      sonic_get_header_value(req->headers, req->headers_count, "upload-id");
  if (upload_id == NULL) {
    respond_error(req, "no upload-id header in publish request \n");
    return;
  }

  char tmp_path[FILE_PATH_SIZE];
  utils_get_node_tmp_path(api_ctx, tmp_path);

  char tmp_upload_path[FILE_PATH_SIZE];
  sprintf(tmp_upload_path, "%s/%s", tmp_path, upload_id);

  if (!dir_exists(tmp_upload_path)) {
    respond_error(req, "upload path does not exist");
    return;
  }

  char upload_info_path[FILE_PATH_SIZE];
  sprintf(upload_info_path, "%s/upload_info.json", tmp_upload_path);

  result_t res_upload_info_str = read_file_to_string(upload_info_path);
  SERVER_ERR(res_upload_info_str);
  char *upload_info_str = VALUE(res_upload_info_str);

  result_t res_upload_info = json_string_to_upload_info(upload_info_str);
  SERVER_ERR(res_upload_info);
  upload_info_t *upload_info = VALUE(res_upload_info);

  free(upload_info_str);

  char tmp_tarball_path[FILE_PATH_SIZE];
  sprintf(tmp_tarball_path, "%s/%s.tar", tmp_path, upload_info->shoggoth_id);

  create_file(tmp_tarball_path);

  char tmp_chunks_path[FILE_PATH_SIZE];
  sprintf(tmp_chunks_path, "%s/chunks", tmp_upload_path);

  for (u64 i = 0; i < upload_info->chunk_count; i++) {
    char chunk_id_str[64];
    sprintf(chunk_id_str, U64_FORMAT_SPECIFIER, i);

    char chunk_path[FILE_PATH_SIZE];
    sprintf(chunk_path, "%s/%s", tmp_chunks_path, chunk_id_str);

    result_t res_file_mapping = map_file(chunk_path);
    SERVER_ERR(res_file_mapping);
    file_mapping_t *file_mapping = VALUE(res_file_mapping);

    result_t res_append =
        append_to_file(tmp_tarball_path, file_mapping->content,
                       (u64)file_mapping->info.st_size);
    SERVER_ERR(res_append);

    unmap_file(file_mapping);
  }

  char tmp_signature_path[FILE_PATH_SIZE];
  sprintf(tmp_signature_path, "%s/signature.txt", tmp_upload_path);

  result_t res_received_signature = read_file_to_string(tmp_signature_path);
  SERVER_ERR(res_received_signature);
  char *received_signature = VALUE(res_received_signature);

  char tmp_fingerprint_path[FILE_PATH_SIZE];
  sprintf(tmp_fingerprint_path, "%s/fingerprint.json", tmp_upload_path);

  result_t res_fingerprint_str = read_file_to_string(tmp_fingerprint_path);
  SERVER_ERR(res_fingerprint_str);
  char *fingerprint_str = VALUE(res_fingerprint_str);

  result_t res_delete = delete_dir(tmp_upload_path);
  SERVER_ERR(res_delete);

  // TODO: lock the file while performing verification

  result_t res_fingerprint = json_string_to_fingerprint(fingerprint_str);
  SERVER_ERR(res_fingerprint);
  fingerprint_t *fingerprint = VALUE(res_fingerprint);

  result_t res_resp =
      process_pin_request(api_ctx, tmp_path, upload_info->shoggoth_id,
                          fingerprint, fingerprint_str, received_signature);
  SERVER_ERR(res_resp);
  sonic_server_response_t *resp = VALUE(res_resp);

  sonic_send_response(req, resp);

  sonic_free_server_response(resp);

  free_fingerprint(fingerprint);

  free_upload_info(upload_info);
  free(fingerprint_str);
  free(received_signature);
}

void api_publish_chunk_route(sonic_server_request_t *req) {
  // FIXME: validate the upload_id

  if (!api_ctx->config->pins.allow_publish) {
    respond_error(req, "this node has disabled publishing profiles");
    return;
  }

  char *upload_id =
      sonic_get_header_value(req->headers, req->headers_count, "upload-id");
  if (upload_id == NULL) {
    respond_error(req, "no upload-id header in publish request");
    return;
  }

  char *chunk_id_str =
      sonic_get_header_value(req->headers, req->headers_count, "chunk-id");
  if (chunk_id_str == NULL) {
    respond_error(req, "no chunk-id header in publish request");
    return;
  }

  char *chunk_size_str =
      sonic_get_header_value(req->headers, req->headers_count, "chunk-size");
  if (chunk_size_str == NULL) {
    respond_error(req, "no chunk-size header in publish request");
    return;
  }

  u64 chunk_id = (u64)atoll(chunk_id_str);

  u64 chunk_size = (u64)atoll(chunk_size_str);

  char tmp_path[FILE_PATH_SIZE];
  utils_get_node_tmp_path(api_ctx, tmp_path);

  char tmp_upload_path[FILE_PATH_SIZE];
  sprintf(tmp_upload_path, "%s/%s", tmp_path, upload_id);

  if (!dir_exists(tmp_upload_path)) {
    respond_error(req, "upload path does not exist");
    return;
  }

  char tmp_chunks_path[FILE_PATH_SIZE];
  sprintf(tmp_chunks_path, "%s/chunks", tmp_upload_path);

  char chunk_path[FILE_PATH_SIZE];
  sprintf(chunk_path, "%s/%s", tmp_chunks_path, chunk_id_str);

  u64 chunk_size_limit = 100000;

  if (req->request_body_size != chunk_size) {
    respond_error(req, "request body size should be = chunk size in header");
    return;
  }

  char upload_info_path[FILE_PATH_SIZE];
  sprintf(upload_info_path, "%s/upload_info.json", tmp_upload_path);

  result_t res_upload_info_str = read_file_to_string(upload_info_path);
  SERVER_ERR(res_upload_info_str);
  char *upload_info_str = VALUE(res_upload_info_str);

  result_t res_upload_info = json_string_to_upload_info(upload_info_str);
  SERVER_ERR(res_upload_info);
  upload_info_t *upload_info = VALUE(res_upload_info);

  free(upload_info_str);

  if (!(chunk_id < upload_info->chunk_count)) {
    respond_error(req, "chunk id is greater than chunk count");
    return;
  }

  if (chunk_id == (upload_info->chunk_count - 1)) {
    if (!(chunk_size <= chunk_size_limit)) {
      respond_error(req, "invalid chunk size");
      return;
    }
  } else {
    if (chunk_size_limit != chunk_size) {
      respond_error(req, "invalid chunk size");
      return;
    }
  }

  free_upload_info(upload_info);

  result_t res_write = write_to_file(chunk_path, req->request_body, chunk_size);
  SERVER_ERR(res_write);

  sonic_server_response_t *resp =
      sonic_new_response(STATUS_200, MIME_TEXT_PLAIN);

  sonic_send_response(req, resp);

  sonic_free_server_response(resp);
}

void add_peer(char *peer_manifest) {
  result_t res_manifest = json_string_to_node_manifest(peer_manifest);
  if (is_err(res_manifest)) {
    return;
  }
  node_manifest_t *manifest = VALUE(res_manifest);

  add_new_peer(api_ctx, manifest->public_host);

  free_node_manifest(manifest);
}

void api_get_dht_route(sonic_server_request_t *req) {
  if (req->request_body_size > 0) {
    char *req_body_str = malloc((req->request_body_size + 1) * sizeof(char));
    strncpy(req_body_str, req->request_body, req->request_body_size);

    req_body_str[req->request_body_size] = '\0';

    add_peer(req_body_str);
    free(req_body_str);
  }

  result_t res_body = db_get_dht_str(api_ctx);
  SERVER_ERR(res_body);
  char *body = VALUE(res_body);

  sonic_server_response_t *resp =
      sonic_new_response(STATUS_200, MIME_APPLICATION_JSON);
  sonic_response_set_body(resp, body, strlen(body));

  sonic_response_add_header(resp, "Access-Control-Allow-Origin", "*");

  sonic_send_response(req, resp);
  sonic_free_server_response(resp);

  free(body);
}

void api_get_pins_route(sonic_server_request_t *req) {
  result_t res_pins_str = db_get_pins_str(api_ctx);
  SERVER_ERR(res_pins_str);
  char *pins_str = VALUE(res_pins_str);

  char *body = pins_str;

  sonic_server_response_t *resp =
      sonic_new_response(STATUS_200, MIME_APPLICATION_JSON);
  sonic_response_set_body(resp, body, strlen(body));

  sonic_response_add_header(resp, "Access-Control-Allow-Origin", "*");

  sonic_send_response(req, resp);
  free(body);
  sonic_free_server_response(resp);
}

void api_get_fingerprint_route(sonic_server_request_t *req) {
  char *shoggoth_id = sonic_get_path_segment(req, "shoggoth_id");

  char runtime_path[FILE_PATH_SIZE];
  utils_get_node_runtime_path(api_ctx, runtime_path);

  char fingerprint_path[FILE_PATH_SIZE];
  sprintf(fingerprint_path, "%s/pins/%s/.shoggoth/fingerprint.json",
          runtime_path, shoggoth_id);

  result_t res_fingerprint_str = read_file_to_string(fingerprint_path);
  SERVER_ERR(res_fingerprint_str);
  char *fingerprint_str = VALUE(res_fingerprint_str);

  char *body = fingerprint_str;

  sonic_server_response_t *resp =
      sonic_new_response(STATUS_200, MIME_APPLICATION_JSON);
  sonic_response_set_body(resp, body, strlen(body));

  sonic_send_response(req, resp);
  free(body);
  sonic_free_server_response(resp);
}

void api_get_manifest_route(sonic_server_request_t *req) {
  if (req->request_body_size > 0) {
    char *req_body_str = malloc((req->request_body_size + 1) * sizeof(char));
    strncpy(req_body_str, req->request_body, req->request_body_size);

    req_body_str[req->request_body_size] = '\0';

    add_peer(req_body_str);
    free(req_body_str);
  }

  result_t res_manifest_json = json_node_manifest_to_json(*api_ctx->manifest);
  SERVER_ERR(res_manifest_json);
  json_t *manifest_json = VALUE(res_manifest_json);

  char *body = json_to_string(manifest_json);
  free_json(manifest_json);

  sonic_server_response_t *resp =
      sonic_new_response(STATUS_200, MIME_APPLICATION_JSON);

  sonic_response_set_body(resp, body, strlen(body));

  sonic_response_add_header(resp, "Access-Control-Allow-Origin", "*");

  sonic_send_response(req, resp);
  sonic_free_server_response(resp);

  free(body);
}

void add_api_routes(node_ctx_t *ctx, sonic_server_t *server) {
  api_ctx = ctx;

  sonic_add_route(server, "/api/clone/{shoggoth_id}", METHOD_GET,
                  api_clone_route);
  sonic_add_route(server, "/api/clone/{shoggoth_id}/{group_name}", METHOD_GET,
                  api_clone_route);
  sonic_add_route(server,
                  "/api/clone/{shoggoth_id}/{group_name}/{resource_name}",
                  METHOD_GET, api_clone_route);

  sonic_add_route(server,
                  "/api/download/{shoggoth_id}/{group_name}/{resource_name}",
                  METHOD_GET, api_download_route);

  sonic_add_route(server, "/api/publish", METHOD_GET,
                  api_negotiate_publish_route);
  sonic_add_route(server, "/api/publish_chunk", METHOD_GET,
                  api_publish_chunk_route);
  sonic_add_route(server, "/api/publish_finish", METHOD_GET,
                  api_publish_finish_route);

  sonic_add_route(server, "/api/get_dht", METHOD_GET, api_get_dht_route);
  sonic_add_route(server, "/api/get_pins", METHOD_GET, api_get_pins_route);
  sonic_add_route(server, "/api/get_fingerprint/{shoggoth_id}", METHOD_GET,
                  api_get_fingerprint_route);
  sonic_add_route(server, "/api/get_manifest", METHOD_GET,
                  api_get_manifest_route);
}
