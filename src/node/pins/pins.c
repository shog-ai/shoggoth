/**** pins.c ****
 *
 *  Copyright (c) 2023 ShogAI - https://shog.ai
 *
 * Part of the Shoggoth project, under the MIT License.
 * See LICENSE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#include "../../json/json.h"
#include "../db/db.h"

#include <assert.h>
#include <stdlib.h>

void download_response_callback(char *data, u64 size, void *user_pointer) {
  char *tmp_tarball_path = (char *)user_pointer;

  append_to_file(tmp_tarball_path, data, size);
}

void free_pins(pins_t *pins) {
  for (u64 i = 0; i < pins->pins_count; i++) {
    free(pins->pins[i]);
  }

  free(pins->pins);

  free(pins);
}

void run_update_script(node_ctx_t *ctx) {
  chdir(ctx->runtime_path);

  char update_script[256];
  sprintf(update_script, "sleep 10 && cd ./node/update/code/update/ && "
                         "sh ./update.sh > ./update_logs.txt &");

  system(update_script);

  chdir(ctx->runtime_path);
}

void update_node_restart(node_ctx_t *ctx) {
  chdir(ctx->runtime_path);

  char restart_script[256];
  sprintf(restart_script, "sleep 20 && chmod +x ./bin/shog && ./bin/shog node "
                          "start -r $(pwd) > ./update_logs.txt &");

  system(restart_script);

  shoggoth_node_exit(0);
}

void auto_update_node(node_ctx_t *ctx) {
  LOG(WARN, "STARTING NODE UPDATE ...");
  run_update_script(ctx);

  LOG(WARN, "RESTARTING NODE SERVICE ...");
  update_node_restart(ctx);
}
