/**** profile.h ****
 *
 *  Copyright (c) 2023 Shoggoth Systems
 *
 * Part of the Shoggoth project, under the MIT License.
 * See LICENCE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#ifndef SHOG_CLIENT_PROFILE
#define SHOG_CLIENT_PROFILE

#include "../client/client.h"
#include "../utils/utils.h"

typedef struct {
  char *public_key;
  char *hash;
  char *resource_name;
  char *shoggoth_id;
  // char *timestamp;
} resource_fingerprint_t;

typedef struct {
  char *public_key;
  char *hash;
  char *group_name;
  char *shoggoth_id;
  // char *timestamp;
} group_fingerprint_t;

typedef struct {
  char *public_key;
  char *shoggoth_id;
  char *hash;
  char *timestamp;
} fingerprint_t;

void free_fingerprint(fingerprint_t *fingerprint);

void free_group_fingerprint(group_fingerprint_t *fingerprint);

void free_resource_fingerprint(resource_fingerprint_t *fingerprint);

result_t validate_authorization(char *tmp_path, char *file_path, char *hash,
                                char *public_key, char *fingerprint_str,
                                char *signature);

result_t validate_profile(char *path);

result_t validate_resource_group(char *path);

result_t validate_git_repo(char *path);

result_t client_init_profile(client_manifest_t *manifest);

result_t client_publish_profile(client_ctx_t *ctx);

#endif
