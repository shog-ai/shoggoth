/**** pin.c ****
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
#include <stdlib.h>

result_t process_pin_request(node_ctx_t *ctx, char *tmp_path, char *shoggoth_id,
                             fingerprint_t *fingerprint, char *fingerprint_str,
                             char *received_signature) {
  char tmp_dir_path[256];
  sprintf(tmp_dir_path, "%s/%s", tmp_path, shoggoth_id);

  char pins_path[256];
  utils_get_node_runtime_path(ctx, pins_path);
  strcat(pins_path, "/pins");

  char tmp_tarball_path[FILE_PATH_SIZE];
  sprintf(tmp_tarball_path, "%s/%s.tar", tmp_path, shoggoth_id);

  result_t valid = validate_authorization(
      tmp_path, tmp_tarball_path, fingerprint->hash, fingerprint->public_key,
      fingerprint_str, received_signature);

  PROPAGATE(valid);

  result_t res_untarball =
      utils_extract_tarball(tmp_tarball_path, tmp_dir_path);
  PROPAGATE(res_untarball);

  result_t profile_dir_valid = validate_profile(tmp_dir_path);
  PROPAGATE(profile_dir_valid);

  char final_dir_path[256];
  utils_get_node_runtime_path(ctx, final_dir_path);
  strcat(final_dir_path, "/pins/");
  strcat(final_dir_path, shoggoth_id);

  char metadata_dir[FILE_PATH_SIZE];
  sprintf(metadata_dir, "%s/.shoggoth", final_dir_path);

  char fingerprint_path[FILE_PATH_SIZE];
  sprintf(fingerprint_path, "%s/fingerprint.json", metadata_dir);

  char signature_path[FILE_PATH_SIZE];
  sprintf(signature_path, "%s/signature.txt", metadata_dir);

  bool is_update = false;
  if (dir_exists(final_dir_path)) {
    is_update = true;
  }

  bool should_update = false;
  if (is_update) {
    result_t res_old_fingerprint_str =
        read_file_to_string(fingerprint_path);
    char *old_fingerprint_str = PROPAGATE(res_old_fingerprint_str);

    result_t res_old_fingerprint =
        json_string_to_fingerprint(old_fingerprint_str);
    fingerprint_t *old_fingerprint = UNWRAP(res_old_fingerprint);

    u64 old_ts = strtoull(old_fingerprint->timestamp, NULL, 10);
    u64 new_ts = strtoull(fingerprint->timestamp, NULL, 10);

    if (strcmp(old_fingerprint->hash, fingerprint->hash) != 0 &&
        new_ts > old_ts) {
      should_update = true;
    }

    free(old_fingerprint_str);
    free_fingerprint(old_fingerprint);
  }

  if (!is_update || (is_update && should_update)) {
    if (is_update) {
      delete_dir(final_dir_path);
    }

    result_t res_untarball =
        utils_extract_tarball(tmp_tarball_path, final_dir_path);
    PROPAGATE(res_untarball);

    result_t res_write = write_to_file(fingerprint_path, fingerprint_str,
                                             strlen(fingerprint_str));
    PROPAGATE(res_write);

    res_write = write_to_file(signature_path, received_signature,
                                    strlen(received_signature));
    PROPAGATE(res_write);

    result_t res_delete = delete_file(tmp_tarball_path);
    PROPAGATE(res_delete);

    res_delete = delete_dir(tmp_dir_path);
    PROPAGATE(res_delete);

    if (!is_update) {
      result_t res_add = db_pins_add_profile(ctx, shoggoth_id);
      PROPAGATE(res_add);
    }

    if (!is_update) {
      LOG(INFO, "NEW PIN PUBLISHED: %s", shoggoth_id);
    } else {
      LOG(INFO, "PIN UPDATED: %s", shoggoth_id);
    }

    if (!is_update) {
      char *body = "publish complete";

      sonic_server_response_t *resp =
          sonic_new_response(STATUS_200, MIME_TEXT_PLAIN);

      sonic_response_set_body(resp, body, strlen(body));

      return OK(resp);
    } else {
      char *body = "update complete";

      sonic_server_response_t *resp =
          sonic_new_response(STATUS_202, MIME_TEXT_PLAIN);

      sonic_response_set_body(resp, body, strlen(body));

      return OK(resp);
    }
  } else {
    delete_file(tmp_tarball_path);
    delete_dir(tmp_dir_path);

    sonic_server_response_t *resp =
        sonic_new_response(STATUS_406, MIME_TEXT_PLAIN);

    char *body = ("Your profile has already been published and no changes were "
                  "detected");

    sonic_response_set_body(resp, body, strlen(body));

    return OK(resp);
  }

  PANIC("no response was returned");

  return ERR("no response was returned");
}