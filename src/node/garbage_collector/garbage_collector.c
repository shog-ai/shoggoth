/**** garbage_collector.c ****
 *
 *  Copyright (c) 2023 Shoggoth Systems
 *
 * Part of the Shoggoth project, under the MIT License.
 * See LICENCE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#include "garbage_collector.h"
#include "../../json/json.h"
#include "../../utils/utils.h"
#include "../db/db.h"
#include "../node.h"
#include "../pins/pins.h"

#include <assert.h>
#include <stdlib.h>

// void *garbage_collector(void *thread_arg) {
//   garbage_collector_thread_args_t *arg =
//       (garbage_collector_thread_args_t *)thread_arg;

//   if (!arg->ctx->config->gc.enable) {
//     LOG(WARN, "Garbage collector disabled");

//     return NULL;
//   }

//   for (;;) {
//     if (arg->ctx->should_exit) {

//       return NULL;
//     }

//     sleep(arg->ctx->config->gc.frequency);

//     if (arg->ctx->should_exit) {

//       return NULL;
//     }

//     LOG(INFO, "Starting garbage collector");

//     result_t res_local_pins_str = db_get_pins_str(arg->ctx);
//     char *local_pins_str = UNWRAP(res_local_pins_str);

//     result_t res_local_pins = json_string_to_pins(local_pins_str);
//     pins_t *local_pins = UNWRAP(res_local_pins);

//     free(local_pins_str);

//     for (u64 i = 0; i < local_pins->pins_count; i++) {
//       char pins_path[256];
//       utils_get_node_runtime_path(arg->ctx, pins_path);
//       strcat(pins_path, "/pins");

//       result_t res_total_pins_size = get_dir_size(pins_path);
//       u64 total_pins_size = UNWRAP_U64(res_total_pins_size);

//       u64 total_storage_limit =
//           (u64)(arg->ctx->config->storage.limit * 1000000000.0); // gigabytes

//       if (total_pins_size < (total_storage_limit / 2)) {
//         LOG(INFO, "Garbage collector returned early: surplus storage");

//         break;
//       }

//       char runtime_path[FILE_PATH_SIZE];
//       utils_get_node_runtime_path(arg->ctx, runtime_path);

//       char fingerprint_path[FILE_PATH_SIZE];
//       sprintf(fingerprint_path, "%s/pins/%s/.shoggoth/fingerprint.json",
//               runtime_path, local_pins->pins[i]);

//       result_t res_fingerprint_str =
//           read_file_to_string(fingerprint_path);
//       char *fingerprint_str = UNWRAP(res_fingerprint_str);

//       result_t res_fingerprint = json_string_to_fingerprint(fingerprint_str);
//       fingerprint_t *fingerprint = UNWRAP(res_fingerprint);

//       free(fingerprint_str);

//       u64 timestamp = strtoull(fingerprint->timestamp, NULL, 0);

//       u64 now = get_timestamp_ms();
//       u64 one_day_ms = 86400000;
//       u64 expiry_duration = arg->ctx->config->gc.profile_expiry *
//                             one_day_ms; // milliseconds in 1 minute

//       if (now > (timestamp + expiry_duration)) {
//         LOG(INFO, "Garbage collector unpinning %s", local_pins->pins[i]);

//         char pin_tarball_path[FILE_PATH_SIZE];
//         sprintf(pin_tarball_path, "%s/pins/%s.tar", runtime_path,
//                 local_pins->pins[i]);

//         char pin_dir_path[FILE_PATH_SIZE];
//         sprintf(pin_dir_path, "%s/pins/%s", runtime_path, local_pins->pins[i]);

//         db_pins_remove_profile(arg->ctx, local_pins->pins[i]);

//         delete_file(pin_tarball_path);
//         delete_dir(pin_dir_path);
//       }

//       free_fingerprint(fingerprint);
//     }

//     free_pins(local_pins);

//     LOG(INFO, "Garbage collector done");
//   }
// }