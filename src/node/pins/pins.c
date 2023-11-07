/**** pins.c ****
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

#include <assert.h>
#include <stdlib.h>

void download_response_callback(char *data, u64 size, void *user_pointer) {
  char *tmp_tarball_path = (char *)user_pointer;

  append_to_file(tmp_tarball_path, data, size);
}

/****
 * downloads a Shoggoth profile from another node
 *
 ****/
result_t download_remote_profile(node_ctx_t *ctx, char *remote_host,
                                 char *shoggoth_id, char *final_dir_path) {

  char tmp_path[512];
  utils_get_node_tmp_path(ctx, tmp_path);

  char tmp_dir_path[256];
  sprintf(tmp_dir_path, "%s/%s", tmp_path, shoggoth_id);

  char tmp_tarball_path[FILE_PATH_SIZE];
  sprintf(tmp_tarball_path, "%s/%s.tar", tmp_path, shoggoth_id);

  result_t res_tmp_tarball_lock =
      acquire_file_lock(tmp_tarball_path, 1000, 10000);
  file_lock_t *tmp_tarball_lock = PROPAGATE(res_tmp_tarball_lock);

  char *user_ptr = strdup(tmp_tarball_path);

  char req_url[FILE_PATH_SIZE];
  sprintf(req_url, "%s/api/clone/%s", remote_host, shoggoth_id);

  sonic_request_t *req = sonic_new_request(METHOD_GET, req_url);
  if (req == NULL) {
    PANIC(sonic_get_error());
  }

  sonic_request_set_response_callback(req, download_response_callback,
                                      user_ptr);

  sonic_response_t *resp = sonic_send_request(req);
  if (resp->failed) {
    delete_file(tmp_tarball_path);
    release_file_lock(tmp_tarball_lock);

    sonic_free_request(req);
    // sonic_free_response(resp); // WARN: resp->err used below

    return ERR("downloading remote pin failed: %s", resp->error);
  }

  char *fingerprint_str =
      sonic_get_header_value(resp->headers, resp->headers_count, "fingerprint");
  if (fingerprint_str == NULL) {
    delete_file(tmp_tarball_path);
    release_file_lock(tmp_tarball_lock);
    sonic_free_request(req);
    sonic_free_response(resp);

    return ERR("no fingerprint header was found in remote clone response");
  }

  char *signature_str =
      sonic_get_header_value(resp->headers, resp->headers_count, "signature");
  if (signature_str == NULL) {
    delete_file(tmp_tarball_path);
    sonic_free_request(req);
    sonic_free_response(resp);

    release_file_lock(tmp_tarball_lock);
    return ERR("no signature header was found in remote clone response");
  }

  free(user_ptr);

  if (resp->status == STATUS_200) {
    char pins_path[256];
    utils_get_node_runtime_path(ctx, pins_path);
    strcat(pins_path, "/pins");

    result_t res_profile_size = get_file_size(tmp_tarball_path);
    u64 profile_size = PROPAGATE_U64(res_profile_size);

    u64 profile_size_limit =
        (u64)(ctx->config->storage.max_profile_size * 1000000.0); // megabytes

    result_t res_total_pins_size = get_dir_size(pins_path);
    u64 total_pins_size = PROPAGATE_U64(res_total_pins_size);

    u64 total_pins_limit =
        (u64)(ctx->config->storage.limit * 1000000000.0); // gigabytes

    if (profile_size > profile_size_limit) {
      delete_file(tmp_tarball_path);

      sonic_free_request(req);
      sonic_free_response(resp);

      LOG(ERROR, "Download remote profile failed: profile too large");

      release_file_lock(tmp_tarball_lock);
      return ERR("profile too large");
    } else if (profile_size + total_pins_size > total_pins_limit) {
      delete_file(tmp_tarball_path);

      sonic_free_request(req);
      sonic_free_response(resp);

      LOG(ERROR, "Download remote profile failed: storage limit exceeded");
      release_file_lock(tmp_tarball_lock);
      return ERR("storage limit exceeded");
    }

    utils_extract_tarball(tmp_tarball_path, tmp_dir_path);

    result_t profile_dir_valid = validate_profile(tmp_dir_path);
    if (is_err(profile_dir_valid)) {
      delete_file(tmp_tarball_path);
      delete_file(tmp_dir_path);

      sonic_free_request(req);
      sonic_free_response(resp);

      release_file_lock(tmp_tarball_lock);
    }

    PROPAGATE(profile_dir_valid);

    result_t res_fingerprint = json_string_to_fingerprint(fingerprint_str);
    fingerprint_t *fingerprint = PROPAGATE(res_fingerprint);

    result_t valid = validate_authorization(
        tmp_path, tmp_tarball_path, fingerprint->hash, fingerprint->public_key,
        fingerprint_str, signature_str);

    if (is_err(valid)) {
      delete_file(tmp_tarball_path);
      delete_dir(tmp_dir_path);

      sonic_free_request(req);
      sonic_free_response(resp);

      release_file_lock(tmp_tarball_lock);
    }
    PROPAGATE(valid);

    utils_extract_tarball(tmp_tarball_path, final_dir_path);

    char metadata_dir[FILE_PATH_SIZE];
    sprintf(metadata_dir, "%s/.shoggoth", final_dir_path);

    char fingerprint_path[FILE_PATH_SIZE];
    sprintf(fingerprint_path, "%s/fingerprint.json", metadata_dir);

    write_to_file(fingerprint_path, fingerprint_str,
                        strlen(fingerprint_str));

    char signature_path[FILE_PATH_SIZE];
    sprintf(signature_path, "%s/signature.txt", metadata_dir);

    write_to_file(signature_path, signature_str, strlen(signature_str));

    delete_file(tmp_tarball_path);
    delete_dir(tmp_dir_path);

    free_fingerprint(fingerprint);
    sonic_free_request(req);
    sonic_free_response(resp);

  } else {
    delete_file(tmp_tarball_path);
    release_file_lock(tmp_tarball_lock);
    sonic_free_request(req);
    sonic_free_response(resp);

    return ERR("Could not download profile: status was not OK");
  }

  release_file_lock(tmp_tarball_lock);

  return OK(NULL);
}

void free_pins(pins_t *pins) {
  for (u64 i = 0; i < pins->pins_count; i++) {
    free(pins->pins[i]);
  }

  free(pins->pins);

  free(pins);
}

void run_update_script(node_ctx_t *ctx) {
  chdir(ctx->runtime_path);

  char update_script[256];
  sprintf(update_script, "sleep 10 && cd ./node/update/code/update/ && "
                         "sh ./update.sh > ./update_logs.txt &");

  system(update_script);

  chdir(ctx->runtime_path);
}

void update_node_restart(node_ctx_t *ctx) {
  chdir(ctx->runtime_path);

  char restart_script[256];
  sprintf(restart_script, "sleep 20 && chmod +x ./bin/shog && ./bin/shog node "
                          "start -r $(pwd) > ./update_logs.txt &");

  system(restart_script);

  shoggoth_node_exit(0);
}

void auto_update_node(node_ctx_t *ctx) {
  LOG(WARN, "STARTING NODE UPDATE ...");
  run_update_script(ctx);

  LOG(WARN, "RESTARTING NODE SERVICE ...");
  update_node_restart(ctx);
}

/****
 * starts an infinite loop that periodically updates the pins. This function
 * runs in a separate new thread
 *
 ****/
void *pins_downloader(void *thread_arg) {
  pins_downloader_thread_args_t *arg =
      (pins_downloader_thread_args_t *)thread_arg;

  if (!arg->ctx->config->pins.enable_downloader) {
    LOG(WARN, "Pin downloader disabled");

    return NULL;
  }

  for (;;) {
    if (arg->ctx->should_exit) {

      return NULL;
    }

    sleep(arg->ctx->config->pins.downloader_frequency);

    if (arg->ctx->should_exit) {

      return NULL;
    }

    result_t res_dht_str = db_get_dht_str(arg->ctx);
    char *dht_str = UNWRAP(res_dht_str);

    result_t res_local_pins_str = db_get_pins_str(arg->ctx);
    char *local_pins_str = UNWRAP(res_local_pins_str);

    result_t res_dht = json_string_to_dht(dht_str);
    dht_t *dht = UNWRAP(res_dht);

    free(dht_str);

    result_t res_local_pins = json_string_to_pins(local_pins_str);
    pins_t *local_pins = UNWRAP(res_local_pins);

    free(local_pins_str);

    for (u64 i = 0; i < dht->items_count; i++) {
      char req_url[FILE_PATH_SIZE];
      sprintf(req_url, "%s/api/get_pins", dht->items[i]->host);

      sonic_request_t *req = sonic_new_request(METHOD_GET, req_url);
      if (req == NULL) {
        PANIC(sonic_get_error());
      }

      sonic_response_t *resp = sonic_send_request(req);
      if (resp->failed) {
        LOG(WARN, "could not get remote pins of %s from %s: %s",
            dht->items[i]->node_id, dht->items[i]->host, resp->error);

        sonic_free_request(req);
        sonic_free_response(resp);

        continue;
      } else if (resp->status != STATUS_200) {
        LOG(WARN, "could not get remote pins: response status was not 200");

        sonic_free_request(req);
        sonic_free_response(resp);

        continue;
      }

      char *response_body_str =
          malloc((resp->response_body_size + 1) * sizeof(char));
      assert(resp->response_body != NULL);

      strncpy(response_body_str, resp->response_body, resp->response_body_size);

      response_body_str[resp->response_body_size] = '\0';

      free(resp->response_body);
      sonic_free_request(req);
      sonic_free_response(resp);

      result_t res_remote_pins = json_string_to_pins(response_body_str);
      pins_t *remote_pins = UNWRAP(res_remote_pins);

      free(response_body_str);

      db_clear_peer_pins(arg->ctx, dht->items[i]->node_id);

      for (u64 k = 0; k < remote_pins->pins_count; k++) {
        db_peer_pins_add_profile(arg->ctx, dht->items[i]->node_id,
                                 remote_pins->pins[k]);

        if (!arg->ctx->config->pins.enable_downloader) {
          continue;
        }

        bool item_exists = false;

        for (u64 w = 0; w < local_pins->pins_count; w++) {
          if (strcmp(local_pins->pins[w], remote_pins->pins[k]) == 0) {
            item_exists = true;
          }
        }

        if (!item_exists) {

          char final_dir_path[256];
          sprintf(final_dir_path, "%s/node/pins/%s", arg->ctx->runtime_path,
                  remote_pins->pins[k]);

          result_t res =
              download_remote_profile(arg->ctx, dht->items[i]->host,
                                      remote_pins->pins[k], final_dir_path);

          if (is_ok(res)) {

            db_pins_add_profile(arg->ctx, remote_pins->pins[k]);

            free_pins(local_pins);

            result_t res_local_pins_str = db_get_pins_str(arg->ctx);
            local_pins_str = UNWRAP(res_local_pins_str);

            result_t res_local_pins = json_string_to_pins(local_pins_str);
            local_pins = UNWRAP(res_local_pins);

            free(local_pins_str);

            LOG(INFO, "PIN DOWNLOADED: %s", remote_pins->pins[k]);

            if (strcmp(arg->ctx->config->update.id, remote_pins->pins[i]) ==
                0) {
              char update_path[FILE_PATH_SIZE];
              utils_get_node_update_path(arg->ctx, update_path);

              result_t res_delete = delete_dir(update_path);
              UNWRAP(res_delete);

              result_t res_copy = copy_dir(final_dir_path, update_path);
              UNWRAP(res_copy);

              auto_update_node(arg->ctx);
            }
          } else {
            LOG(ERROR, "download remote profile %s failed: %s",
                remote_pins->pins[k], res.error_message);
          }

          free_result(res);
        } else {
        }
      }

      free_pins(remote_pins);
    }

    free_pins(local_pins);
    free_dht(dht);
  }

  return NULL;
}

void *pins_updater(void *thread_arg) {
  pins_updater_thread_args_t *arg = (pins_updater_thread_args_t *)thread_arg;

  if (!arg->ctx->config->pins.enable_updater) {
    LOG(WARN, "Pin updater disabled");

    return NULL;
  }

  if (!arg->ctx->config->update.enable) {
    LOG(WARN, "auto-update disabled");
  }

  for (;;) {
    if (arg->ctx->should_exit) {

      return NULL;
    }

    sleep(arg->ctx->config->pins.updater_frequency);

    if (arg->ctx->should_exit) {

      return NULL;
    }

    result_t res_local_pins_str = db_get_pins_str(arg->ctx);
    char *local_pins_str = UNWRAP(res_local_pins_str);

    result_t res_local_pins = json_string_to_pins(local_pins_str);
    pins_t *local_pins = UNWRAP(res_local_pins);

    for (u64 i = 0; i < local_pins->pins_count; i++) {
      result_t res_peers_with_pin =
          db_get_peers_with_pin(arg->ctx, local_pins->pins[i]);
      char *peers_with_pin = UNWRAP(res_peers_with_pin);

      result_t res_peers = json_string_to_dht(peers_with_pin);
      dht_t *peers = UNWRAP(res_peers);

      free(peers_with_pin);

      for (u64 k = 0; k < peers->items_count; k++) {

        char req_url[FILE_PATH_SIZE];
        sprintf(req_url, "%s/api/get_fingerprint/%s", peers->items[k]->host,
                local_pins->pins[i]);

        sonic_request_t *req = sonic_new_request(METHOD_GET, req_url);
        if (req == NULL) {
          PANIC(sonic_get_error());
        }

        sonic_response_t *resp = sonic_send_request(req);
        if (resp->failed) {
          LOG(WARN, "could not get fingerprint for pin: %s", resp->error);

          sonic_free_request(req);
          sonic_free_response(resp);

          continue;
        } else if (resp->status != STATUS_200) {
          LOG(WARN,
              "could not get fingerprint for pin: response status was not 200");

          sonic_free_request(req);
          sonic_free_response(resp);

          continue;
        }

        char *response_body_str =
            malloc((resp->response_body_size + 1) * sizeof(char));
        assert(resp->response_body != NULL);

        strncpy(response_body_str, resp->response_body,
                resp->response_body_size);

        response_body_str[resp->response_body_size] = '\0';

        free(resp->response_body);
        sonic_free_request(req);
        sonic_free_response(resp);

        result_t res_remote_fingerprint =
            json_string_to_fingerprint(response_body_str);
        fingerprint_t *remote_fingerprint = UNWRAP(res_remote_fingerprint);

        free(response_body_str);

        char runtime_path[FILE_PATH_SIZE];
        utils_get_node_runtime_path(arg->ctx, runtime_path);

        char local_fingerprint_path[FILE_PATH_SIZE];
        sprintf(local_fingerprint_path, "%s/pins/%s/.shoggoth/fingerprint.json",
                runtime_path, local_pins->pins[i]);

        result_t res_local_fingerprint_str =
            read_file_to_string(local_fingerprint_path);
        char *local_fingerprint_str = UNWRAP(res_local_fingerprint_str);

        result_t res_local_fingerprint =
            json_string_to_fingerprint(local_fingerprint_str);
        fingerprint_t *local_fingerprint = UNWRAP(res_local_fingerprint);

        free(local_fingerprint_str);

        u64 remote_timestamp = strtoull(remote_fingerprint->timestamp, NULL, 0);
        u64 local_timestamp = strtoull(local_fingerprint->timestamp, NULL, 0);

        if ((strcmp(local_fingerprint->hash, remote_fingerprint->hash) != 0) &&
            remote_timestamp > local_timestamp) {
          LOG(INFO, "REMOTE PIN WAS UPDATED");

          char final_dir_path[256];
          sprintf(final_dir_path, "%s/node/pins/%s", arg->ctx->runtime_path,
                  local_pins->pins[i]);

          result_t res =
              download_remote_profile(arg->ctx, peers->items[k]->host,
                                      local_pins->pins[i], final_dir_path);

          if (is_err(res)) {
            LOG(WARN, "could not download remote profile: %s",
                res.error_message);

          } else {
            LOG(INFO, "PIN UPDATED: %s", local_pins->pins[i]);

            if (arg->ctx->config->update.enable) {
              if (strcmp(arg->ctx->config->update.id, local_pins->pins[i]) ==
                  0) {
                char update_path[FILE_PATH_SIZE];
                utils_get_node_update_path(arg->ctx, update_path);

                result_t res_delete = delete_dir(update_path);
                UNWRAP(res_delete);

                result_t res_copy = copy_dir(final_dir_path, update_path);
                UNWRAP(res_copy);

                auto_update_node(arg->ctx);
              }
            }
          }

          free_result(res);
        }

        free_fingerprint(remote_fingerprint);
        free_fingerprint(local_fingerprint);
      }

      free_dht(peers);
    }

    free(local_pins_str);
    free_pins(local_pins);
  }
}
