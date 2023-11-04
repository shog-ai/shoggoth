/**** json.h ****
 *
 *  Copyright (c) 2023 Shoggoth Systems
 *
 * Part of the Shoggoth project, under the MIT License.
 * See LICENCE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#ifndef SHOG_JSON
#define SHOG_JSON

#include "../client/client.h"
#include "../node/dht/dht.h"
#include "../node/node.h"
#include "../node/server/server.h"
#include "../profile/profile.h"

typedef void *json_t;

result_t json_string_to_upload_info(char *info_str);

result_t json_known_nodes_to_json(known_nodes_t known_nodes);

result_t json_string_to_known_nodes(char *nodes_str);

result_t json_string_to_fingerprint(char *fingerprint_str);

result_t json_fingerprint_to_json(fingerprint_t fingerprint);

result_t json_string_to_resource_fingerprint(char *fingerprint_str);

result_t json_resource_fingerprint_to_json(resource_fingerprint_t fingerprint);

result_t json_string_to_group_fingerprint(char *fingerprint_str);

result_t json_group_fingerprint_to_json(group_fingerprint_t fingerprint);

void free_json(json_t *json);

char *json_to_string(json_t *json);

result_t json_client_manifest_to_json(client_manifest_t manifest);

result_t json_node_manifest_to_json(node_manifest_t manifest);

result_t json_string_to_client_manifest(char *manifest_str);

result_t json_string_to_pins(char *pins_str);

result_t json_string_to_node_manifest(char *manifest_str);

result_t json_string_to_dht(char *dht_str);

result_t json_dht_item_to_json(dht_item_t item);

result_t json_dht_to_json(dht_t dht);

#endif
