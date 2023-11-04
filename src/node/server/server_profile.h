/**** server_profile.h ****
 *
 *  Copyright (c) 2023 Shoggoth Systems
 *
 * Part of the Shoggoth project, under the MIT License.
 * See LICENCE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#ifndef SHOG_NODE_SERVER_PROFILE_H
#define SHOG_NODE_SERVER_PROFILE_H

#include "../node.h"
#include "../../include/sonic.h"

void add_profile_routes(node_ctx_t *ctx, sonic_server_t *server);

#endif
