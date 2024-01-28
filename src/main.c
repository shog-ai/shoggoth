/**** main.c ****
 *
 *  Copyright (c) 2023 Shoggoth Systems
 *
 * Part of the Shoggoth project, under the MIT License.
 * See LICENSE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/
#include <netlibc.h>
#include <netlibc/error.h>
#include <netlibc/fs.h>
#include <netlibc/log.h>
#include <netlibc/string.h>

#include "./args/args.h"
#include "const.h"
#include "node/node.h"

#include <stdlib.h>

#ifndef VERSION
#define VERSION "0.0.0"
#endif

int main(int argc, char **argv) {
  result_t res_args = args_parse(argc, argv);
  if (!is_ok(res_args)) {
    EXIT(1, res_args.error_message);
  }

  args_t *args = UNWRAP(res_args);

  if (args->help || args->no_args) {
    printf("Shoggoth - %s\n%s", VERSION, SHOG_HELP);

    exit(0);
  } else if (args->version) {
    printf("Shoggoth -  %s\n%s", VERSION, SHOG_VERSION);

    exit(0);
  }

  if (!args->has_command) {
    EXIT(1, "no command was found in arguments");
  } else if (args->invalid_command != NULL) {
    EXIT(1, "invalid command: %s", args->invalid_command);
  }

  result_t res = handle_node_session(args);
  UNWRAP(res);

  return 0;
}
