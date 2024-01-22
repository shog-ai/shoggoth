/**** db.h ****
 *
 *  Copyright (c) 2023 Shoggoth Systems
 *
 * Part of the Shoggoth project, under the MIT License.
 * See LICENCE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#ifndef NODE_DB_H
#define NODE_DB_H

#include "../dht/dht.h"

result_t db_increment_unreachable_count(node_ctx_t *ctx, char *node_id);

result_t db_reset_unreachable_count(node_ctx_t *ctx, char *node_id);

result_t db_pins_remove_resource(node_ctx_t *ctx, char *shoggoth_id);

result_t db_get_unreachable_count(node_ctx_t *ctx, char *node_id);

result_t db_get_peers_with_pin(node_ctx_t *ctx, char *node_id);

result_t db_clear_peer_pins(node_ctx_t *ctx, char *node_id);

result_t db_peer_pins_add_resource(node_ctx_t *ctx, char *node_id,
                                   char *shoggoth_id);

void launch_db(node_ctx_t *ctx);

bool db_dht_item_exists(node_ctx_t *ctx, char *node_id);

result_t db_dht_add_item(node_ctx_t *ctx, dht_item_t *item);

result_t db_dht_remove_item(node_ctx_t *ctx, char *node_id);

result_t db_pins_add_resource(node_ctx_t *ctx, char *shoggoth_id, char *label);

result_t db_get_dht_str(node_ctx_t *ctx);

result_t db_verify_data(node_ctx_t *ctx);

result_t db_get_pins_str(node_ctx_t *ctx);

#endif
