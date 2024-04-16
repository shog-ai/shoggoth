/**** toml.c ****
 *
 *  Copyright (c) 2023 ShogAI - https://shog.ai
 *
 * Part of the Shoggoth project, under the MIT License.
 * See LICENSE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#include "../include/tomlc.h"
#include "../node/node.h"
#include "../utils/utils.h"

#include "toml.h"

#include <stdlib.h>

#include <netlibc/mem.h>

result_t toml_string_to_node_config(char *config_str) {
  node_config_t *config = ncalloc(1, sizeof(node_config_t));

  char errbuf[200];
  toml_table_t *config_toml = toml_parse(config_str, errbuf, sizeof(errbuf));
  if (config_toml == NULL) {
    PANIC("TOML: Could not parse config");
  }

  // NETWORK TABLE
  toml_table_t *network_table = toml_table_in(config_toml, "network");
  if (network_table == NULL) {
    PANIC("TOML: Could not parse network table \n");
  }

  toml_datum_t network_host = toml_string_in(network_table, "host");
  config->network.host = strdup(network_host.u.s);
  nfree(network_host.u.s);

  toml_datum_t network_port = toml_int_in(network_table, "port");
  config->network.port = (u16)network_port.u.i;

  toml_datum_t network_public_host =
      toml_string_in(network_table, "public_host");
  config->network.public_host = strdup(network_public_host.u.s);
  nfree(network_public_host.u.s);

  toml_datum_t network_allow_private_network =
      toml_bool_in(network_table, "allow_private_network");
  config->network.allow_private_network = network_allow_private_network.u.b;

  // API TABLE
  toml_table_t *api_table = toml_table_in(config_toml, "api");
  if (api_table == NULL) {
    PANIC("TOML: Could not parse api table \n");
  }

  toml_datum_t api_enable = toml_bool_in(api_table, "enable");
  config->api.enable = api_enable.u.b;

  toml_datum_t api_rate_limiter_requests =
      toml_int_in(api_table, "rate_limiter_requests");
  config->api.rate_limiter_requests = (u64)api_rate_limiter_requests.u.i;

  toml_datum_t api_rate_limiter_duration =
      toml_int_in(api_table, "rate_limiter_duration");
  config->api.rate_limiter_duration = (u64)api_rate_limiter_duration.u.i;

  // PEERS TABLE
  toml_table_t *peers_table = toml_table_in(config_toml, "peers");
  if (peers_table == NULL) {
    PANIC("TOML: Could not parse peers table \n");
  }

  toml_array_t *bootstrap_peers_toml =
      toml_array_in(peers_table, "bootstrap_peers");
  char **bootstrap_peers = NULL;
  u64 bootstrap_peers_count = 0;
  for (int i = 0;; i++) {
    toml_datum_t peer = toml_string_at(bootstrap_peers_toml, i);
    if (!peer.ok) {
      break;
    }

    bootstrap_peers_count++;

    bootstrap_peers =
        nrealloc(bootstrap_peers, bootstrap_peers_count * sizeof(char *));

    bootstrap_peers[bootstrap_peers_count - 1] = strdup(peer.u.s);

    nfree(peer.u.s);
  }

  config->peers.bootstrap_peers = bootstrap_peers;
  config->peers.bootstrap_peers_count = bootstrap_peers_count;

  // STORAGE TABLE
  toml_table_t *storage_table = toml_table_in(config_toml, "storage");
  if (storage_table == NULL) {
    PANIC("TOML: Could not parse storage table \n");
  }

  toml_datum_t storage_max_resource_size =
      toml_double_in(storage_table, "max_resource_size");
  config->storage.max_resource_size = (f64)storage_max_resource_size.u.d;

  toml_datum_t storage_limit = toml_double_in(storage_table, "limit");
  config->storage.limit = (f64)storage_limit.u.d;

  // TUNNEL TABLE
  toml_table_t *tunnel_table = toml_table_in(config_toml, "tunnel");
  if (tunnel_table == NULL) {
    PANIC("TOML: Could not parse tunnel table \n");
  }

  toml_datum_t tunnel_enable = toml_bool_in(tunnel_table, "enable");
  config->tunnel.enable = tunnel_enable.u.b;

  toml_datum_t tunnel_server = toml_string_in(tunnel_table, "server");
  config->tunnel.server = strdup(tunnel_server.u.s);
  nfree(tunnel_server.u.s);

  // EXPLORER TABLE
  toml_table_t *explorer_table = toml_table_in(config_toml, "explorer");
  if (explorer_table == NULL) {
    PANIC("TOML: Could not parse explorer table \n");
  }

  toml_datum_t explorer_enable = toml_bool_in(explorer_table, "enable");
  config->explorer.enable = explorer_enable.u.b;

  // DB TABLE
  toml_table_t *db_table = toml_table_in(config_toml, "db");
  if (db_table == NULL) {
    PANIC("TOML: Could not parse db table \n");
  }

  toml_datum_t db_host = toml_string_in(db_table, "host");
  config->db.host = strdup(db_host.u.s);
  nfree(db_host.u.s);

  toml_datum_t db_port = toml_int_in(db_table, "port");
  config->db.port = (u16)db_port.u.i;

  // DHT TABLE
  toml_table_t *dht_table = toml_table_in(config_toml, "dht");
  if (dht_table == NULL) {
    PANIC("TOML: Could not parse dht table \n");
  }

  toml_datum_t dht_enable_updater = toml_bool_in(dht_table, "enable_updater");
  config->dht.enable_updater = dht_enable_updater.u.b;

  toml_datum_t dht_updater_frequency =
      toml_int_in(dht_table, "updater_frequency");
  config->dht.updater_frequency = (u32)dht_updater_frequency.u.i;

  toml_free(config_toml);

  return OK(config);
}
