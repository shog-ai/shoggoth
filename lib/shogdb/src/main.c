/**** main.c ****
 *
 *  Copyright (c) 2023 Shoggoth Systems - https://shoggoth.systems
 *
 * Part of the ShogDB database, under the MIT License.
 * See LICENCE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#include "../include/tomlc.h"
#include "./db/db.h"

#include <netlibc.h>
#include <netlibc/error.h>
#include <netlibc/fs.h>

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

result_t parse_config(char *config_str) {
  db_config_t *config = calloc(1, sizeof(db_config_t));

  char errbuf[200];
  toml_table_t *config_toml = toml_parse(config_str, errbuf, sizeof(errbuf));
  if (config_toml == NULL) {
    return ERR("TOML: Could not parse config");
  }

  // NETWORK TABLE
  toml_table_t *network_table = toml_table_in(config_toml, "network");
  if (network_table == NULL) {
    return ERR("TOML: Could not parse network table \n");
  }

  toml_datum_t network_host = toml_string_in(network_table, "host");
  config->network.host = strdup(network_host.u.s);
  free(network_host.u.s);

  toml_datum_t network_port = toml_int_in(network_table, "port");
  config->network.port = (u16)network_port.u.i;

  // SAVE TABLE
  toml_table_t *save_table = toml_table_in(config_toml, "save");
  if (save_table == NULL) {
    return ERR("TOML: Could not parse save table \n");
  }

  toml_datum_t save_path = toml_string_in(save_table, "path");
  config->save.path = strdup(save_path.u.s);
  free(save_path.u.s);

  toml_datum_t save_interval = toml_int_in(save_table, "interval");
  config->save.interval = (u64)save_interval.u.i;

  toml_free(config_toml);

  return OK(config);
}

int main() {
  result_t res_config_str = read_file_to_string("./dbconfig.toml");
  char *config_str = UNWRAP(res_config_str);

  result_t res_config = parse_config(config_str);
  db_config_t *config = UNWRAP(res_config);

  db_ctx_t *db_ctx = new_db(config);

  start_db(db_ctx);

  free_db(db_ctx);
}