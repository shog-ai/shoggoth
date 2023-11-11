/**** toml.c ****
 *
 *  Copyright (c) 2023 Shoggoth Systems
 *
 * Part of the Shoggoth project, under the MIT License.
 * See LICENCE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#include "../client/client.h"
#include "../include/tomlc.h"
#include "../node/node.h"
#include "../utils/utils.h"

#include "toml.h"

#include <stdlib.h>

result_t toml_string_to_node_config(char *config_str) {
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

  node_config_t *config = calloc(1, sizeof(node_config_t));

  toml_datum_t network_host = toml_string_in(network_table, "host");
  config->network.host = strdup(network_host.u.s);
  free(network_host.u.s);

  toml_datum_t network_port = toml_int_in(network_table, "port");
  config->network.port = (u16)network_port.u.i;

  toml_datum_t network_public_host =
      toml_string_in(network_table, "public_host");
  config->network.public_host = strdup(network_public_host.u.s);
  free(network_public_host.u.s);

  toml_datum_t network_allow_private_network =
      toml_bool_in(network_table, "allow_private_network");
  config->network.allow_private_network = network_allow_private_network.u.b;

  // GARBAGE COLLECTOR TABLE
  toml_table_t *gc_table = toml_table_in(config_toml, "garbage_collector");
  if (gc_table == NULL) {
    PANIC("TOML: Could not parse garbage collector table \n");
  }

  toml_datum_t gc_frequency = toml_int_in(gc_table, "frequency");
  config->gc.frequency = (u32)gc_frequency.u.i;

  toml_datum_t gc_enable = toml_bool_in(gc_table, "enable");
  config->gc.enable = gc_enable.u.b;

  toml_datum_t gc_profile_expiry = toml_int_in(gc_table, "profile_expiry");
  config->gc.profile_expiry = (u64)gc_profile_expiry.u.i;

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
        realloc(bootstrap_peers, bootstrap_peers_count * sizeof(char *));

    bootstrap_peers[bootstrap_peers_count - 1] = strdup(peer.u.s);

    free(peer.u.s);
  }

  config->peers.bootstrap_peers = bootstrap_peers;
  config->peers.bootstrap_peers_count = bootstrap_peers_count;

  // STORAGE TABLE
  toml_table_t *storage_table = toml_table_in(config_toml, "storage");
  if (storage_table == NULL) {
    PANIC("TOML: Could not parse storage table \n");
  }

  toml_datum_t storage_max_profile_size =
      toml_double_in(storage_table, "max_profile_size");
  config->storage.max_profile_size = (f64)storage_max_profile_size.u.d;

  toml_datum_t storage_limit = toml_double_in(storage_table, "limit");
  config->storage.limit = (f64)storage_limit.u.d;

  // UPDATE TABLE
  toml_table_t *update_table = toml_table_in(config_toml, "update");
  if (update_table == NULL) {
    PANIC("TOML: Could not parse update table \n");
  }

  toml_datum_t update_enable = toml_bool_in(update_table, "enable");
  config->update.enable = update_enable.u.b;

  toml_datum_t update_id = toml_string_in(update_table, "id");
  config->update.id = strdup(update_id.u.s);
  free(update_id.u.s);

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
  free(db_host.u.s);

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

  // PINS TABLE
  toml_table_t *pins_table = toml_table_in(config_toml, "pins");
  if (pins_table == NULL) {
    PANIC("TOML: Could not parse pins table \n");
  }

  toml_datum_t pins_allow_publish = toml_bool_in(pins_table, "allow_publish");
  config->pins.allow_publish = pins_allow_publish.u.b;

  toml_datum_t pins_enable_downloader =
      toml_bool_in(pins_table, "enable_downloader");
  config->pins.enable_downloader = pins_enable_downloader.u.b;

  toml_datum_t pins_enable_updater = toml_bool_in(pins_table, "enable_updater");
  config->pins.enable_updater = pins_enable_updater.u.b;

  toml_datum_t pins_updater_frequency =
      toml_int_in(pins_table, "updater_frequency");
  config->pins.updater_frequency = (u32)pins_updater_frequency.u.i;

  toml_datum_t pins_downloader_frequency =
      toml_int_in(pins_table, "downloader_frequency");
  config->pins.downloader_frequency = (u32)pins_downloader_frequency.u.i;

  toml_free(config_toml);

  return OK(config);
}
