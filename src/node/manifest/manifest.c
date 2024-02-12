/**** manifest.c ****
 *
 *  Copyright (c) 2023 ShogAI - https://shog.ai
 *
 * Part of the Shoggoth project, under the MIT License.
 * See LICENSE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#include "../../json/json.h"
#include "../../openssl/openssl.h"

#include <stdlib.h>

result_t node_id_from_public_key(char *public_key_string, char *node_id_output) {
  char node_id_string[512];
  result_t res_public_key_hash =
      openssl_hash_data(public_key_string, strlen(public_key_string));
  char *public_key_hash = PROPAGATE(res_public_key_hash);

  strcpy(node_id_string, "SHOGN");
  strcat(node_id_string, &public_key_hash[32]);

  free(public_key_hash);

  strcpy(node_id_output, node_id_string);
  return OK(NULL);
 }


/****
 * creates a bew node manifest as a json string derived from the public key and
 * public host of the node
 *
 ****/
result_t generate_node_manifest(char *public_key_string, char *public_host,
                                bool has_explorer, char *version) {
  char *stripped_public_key = utils_strip_public_key(public_key_string);

  node_manifest_t manifest = {0};
  manifest.public_host = public_host;
  manifest.public_key = stripped_public_key;
  manifest.has_explorer = has_explorer;
  manifest.version = version;

  char node_id_string[512];
  result_t node_id_result = node_id_from_public_key(stripped_public_key, node_id_string);
  PROPAGATE(node_id_result);

  manifest.node_id = node_id_string;

  result_t res_manifest_json = json_node_manifest_to_json(manifest);
  json_t *manifest_json = PROPAGATE(res_manifest_json);

  char *manifest_string = json_to_string(manifest_json);

  free(stripped_public_key);
  free_json(manifest_json);

  return OK(manifest_string);
}

void free_node_manifest(node_manifest_t *manifest) {
  free(manifest->public_host);
  free(manifest->public_key);
  free(manifest->node_id);
  free(manifest->version);

  free(manifest);
}
