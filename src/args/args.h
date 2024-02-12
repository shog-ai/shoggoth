/**** args.h ****
 *
 *  Copyright (c) 2023 ShogAI - https://shog.ai
 *
 * Part of the Shoggoth project, under the MIT License.
 * See LICENSE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#ifndef CLIENT_ARGS_H
#define CLIENT_ARGS_H

#include "../utils/utils.h"

// typedef enum { COMMAND_NONE, COMMAND_CLIENT, COMMAND_NODE } command_t;

typedef struct {
  bool help;
  bool version;
  bool set_config;
  char *config_path;
  bool no_args;
  bool has_invalid_flag;
  char *invalid_flag;
  bool set_runtime_path;
  char *runtime_path;

  bool has_command;
  char *command;
  char *invalid_command;
  bool has_command_arg;
  char *command_arg;
  bool has_subcommand_arg;
  char *subcommand_arg;
} args_t;

result_t args_parse(const int argc, char **argv);

#endif
