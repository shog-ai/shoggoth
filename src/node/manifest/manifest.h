/**** manifest.h ****
 *
 *  Copyright (c) 2023 ShogAI - https://shog.ai
 *
 * Part of the Shoggoth project, under the MIT License.
 * See LICENSE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#ifndef SHOG_NODE_MANIFEST_H
#define SHOG_NODE_MANIFEST_H

result_t node_id_from_public_key(char *public_key_string, char *node_id_output);

result_t generate_node_manifest(char *public_key_path, char *public_host,
                                bool has_explorer, char *version);

void free_node_manifest(node_manifest_t *manifest);

#endif
