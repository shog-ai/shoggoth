/**** client.c ****
 *
 *  Copyright (c) 2023 Shoggoth Systems
 *
 * Part of the Shoggoth project, under the MIT License.
 * See LICENCE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#include "../const.h"
#include "../json/json.h"
#include "../openssl/openssl.h"
#include "clone.h"

#include <assert.h>
#include <stdlib.h>

result_t add_node_to_known_nodes(client_ctx_t *ctx, char *new_node) {
  ctx->known_nodes->nodes =
      realloc(ctx->known_nodes->nodes,
              (ctx->known_nodes->nodes_count + 1) * sizeof(char *));

  ctx->known_nodes->nodes[ctx->known_nodes->nodes_count] = strdup(new_node);

  ctx->known_nodes->nodes_count++;

  char client_runtime_path[FILE_PATH_SIZE];
  utils_get_client_runtime_path(ctx, client_runtime_path);

  char known_nodes_path[FILE_PATH_SIZE];
  sprintf(known_nodes_path, "%s/known_nodes.json", client_runtime_path);

  if (!utils_file_exists(known_nodes_path)) {
    return ERR("known nodes file `%s` does not exist", known_nodes_path);
  }

  result_t res_known_nodes_json = json_known_nodes_to_json(*ctx->known_nodes);
  json_t *known_nodes_json = PROPAGATE(res_known_nodes_json);

  char *known_nodes_str = json_to_string(known_nodes_json);
  free_json(known_nodes_json);
  utils_write_to_file(known_nodes_path, known_nodes_str,
                      strlen(known_nodes_str));
  free(known_nodes_str);

  return OK(NULL);
}

result_t merge_node_dht_to_known_nodes(client_ctx_t *ctx, char *node_host) {
  char req_url[FILE_PATH_SIZE];
  sprintf(req_url, "%s/api/get_dht", node_host);

  sonic_request_t *req = sonic_new_request(METHOD_GET, req_url);
  if (req == NULL) {
    return ERR(sonic_get_error());
  }

  sonic_response_t *resp = sonic_send_request(req);
  if (resp->failed) {
    LOG(WARN, "could not get remote dht: %s", resp->error);

    char error_msg[256];
    strncpy(error_msg, resp->error, strlen(resp->error));
    error_msg[strlen(resp->error)] = '\0';

    sonic_free_request(req);
    sonic_free_response(resp);

    return ERR("could not get remote dht: %s", error_msg);
  }

  if (resp->failed) {
    return ERR("request failed: %s", resp->error);
  }

  char *response_body_str =
      malloc((resp->response_body_size + 1) * sizeof(char));
  assert(resp->response_body != NULL);

  strncpy(response_body_str, resp->response_body, resp->response_body_size);

  response_body_str[resp->response_body_size] = '\0';

  free(resp->response_body);

  sonic_free_request(req);
  sonic_free_response(resp);

  result_t res_remote_dht = json_string_to_dht(response_body_str);
  dht_t *remote_dht = PROPAGATE(res_remote_dht);

  free(response_body_str);

  for (u64 i = 0; i < remote_dht->items_count; i++) {
    char *remote_host = remote_dht->items[i]->host;

    bool item_exists = false;

    for (u64 w = 0; w < ctx->known_nodes->nodes_count; w++) {
      char *exists_node = ctx->known_nodes->nodes[w];

      if (strcmp(exists_node, remote_host) == 0) {
        item_exists = true;
      }
    }

    if (!item_exists) {
      LOG(INFO, "DISCOVERED NEW NODE: %s", remote_host);

      add_node_to_known_nodes(ctx, remote_host);

    } else {
    }
  }

  free_dht(remote_dht);

  return OK(NULL);
}

/****U
 * chooses a suitable known node and copies its host address to node_host
 *
 ****/
result_t client_delegate_node(client_ctx_t *ctx, char node_host[]) {
  if (ctx->known_nodes->nodes_count == 0) {
    return ERR("ERROR: No known nodes");
  }

  u64 random_index = utils_random_number(0, ctx->known_nodes->nodes_count - 1);

  // LOG(INFO, "RANDOM INDEX: %lu", random_index);

  char *delegated_node = ctx->known_nodes->nodes[random_index];

  LOG(INFO, "Delegated node: %s", delegated_node);

  result_t res = merge_node_dht_to_known_nodes(ctx, delegated_node);
  if (is_err(res)) {
    LOG(ERROR, res.error_message);
  }
  free_result(res);

  strcpy(node_host, delegated_node);

  return OK(NULL);
}

/****
 * uses openssl to generate a new RSA key pair and saves them in their
 * respective paths
 *
 ****/
result_t generate_client_key_pair(char *keys_path, char *public_key_path,
                                  char *private_key_path) {
  printf("%s", CLIENT_KEYS_WARNING);

  getchar();

  result_t res = openssl_generate_key_pair(private_key_path, public_key_path);
  PROPAGATE(res);

  LOG(INFO, "Key pair generated successfully in %s.\n", keys_path);

  return OK(NULL);
}

/****
 * generates a manifest json string
 *
 ****/
result_t generate_client_manifest(char *public_key_path) {
  result_t res_public_key_string = utils_read_file_to_string(public_key_path);
  char *public_key_string = PROPAGATE(res_public_key_string);

  char *stripped_public_key = utils_strip_public_key(public_key_string);

  free(public_key_string);

  client_manifest_t manifest = {0};

  manifest.public_key = stripped_public_key;

  char shoggoth_id_string[SHOGGOTH_ID_SIZE];

  result_t res_public_key_hash =
      openssl_hash_data(stripped_public_key, strlen(stripped_public_key));
  char *public_key_hash = PROPAGATE(res_public_key_hash);

  sprintf(shoggoth_id_string, "SHOG%s", &public_key_hash[32]);

  free(public_key_hash);

  manifest.shoggoth_id = shoggoth_id_string;

  result_t res_manifest_json = json_client_manifest_to_json(manifest);
  json_t *manifest_json = PROPAGATE(res_manifest_json);

  char *manifest_string = json_to_string(manifest_json);

  free_json(manifest_json);
  free(stripped_public_key);

  return OK(manifest_string);
}

/****
 * Initializes the system environment and sets up the client runtime
 *
 ****/
result_t init_client_runtime(client_ctx_t *ctx, args_t *args) {
  char client_runtime_path[FILE_PATH_SIZE];
  utils_get_client_runtime_path(ctx, client_runtime_path);

  char keys_path[FILE_PATH_SIZE];
  sprintf(keys_path, "%s/keys", client_runtime_path);

  char private_key_path[FILE_PATH_SIZE];
  sprintf(private_key_path, "%s/private.txt", keys_path);

  char public_key_path[FILE_PATH_SIZE];
  sprintf(public_key_path, "%s/public.txt", keys_path);

  if (!utils_dir_exists(keys_path)) {
    utils_create_dir(keys_path);
  }

  if (!utils_keys_exist(keys_path)) {
    result_t res_gen =
        generate_client_key_pair(keys_path, public_key_path, private_key_path);
    PROPAGATE(res_gen);
  }

  char config_path[FILE_PATH_SIZE];

  if (args->set_config) {
    sprintf(config_path, "%s", args->config_path);
  } else {
    sprintf(config_path, "%s/config.toml", client_runtime_path);
  }

  if (!utils_file_exists(config_path)) {
    return ERR("Config file `%s` does not exist \n", config_path);
  }

  char known_nodes_path[FILE_PATH_SIZE];
  sprintf(known_nodes_path, "%s/known_nodes.json", client_runtime_path);

  if (!utils_file_exists(known_nodes_path)) {
    return ERR("known nodes file `%s` does not exist \n", known_nodes_path);
  }

  result_t res_known_nodes_str = utils_read_file_to_string(known_nodes_path);
  char *known_nodes_str = PROPAGATE(res_known_nodes_str);

  result_t res_known_nodes = json_string_to_known_nodes(known_nodes_str);
  known_nodes_t *known_nodes = PROPAGATE(res_known_nodes);

  free(known_nodes_str);

  ctx->known_nodes = known_nodes;

  result_t res_manifest_str = generate_client_manifest(public_key_path);
  char *manifest_str = PROPAGATE(res_manifest_str);

  result_t res_manifest = json_string_to_client_manifest(manifest_str);
  client_manifest_t *manifest = PROPAGATE(res_manifest);

  free(manifest_str);

  ctx->manifest = manifest;

  return OK(NULL);
}

/****
 * constructs a new client_ctx_t
 *
 ****/
client_ctx_t *new_client_ctx() {
  client_ctx_t *ctx = calloc(1, sizeof(client_ctx_t));
  assert(ctx != NULL);

  ctx->runtime_path = NULL;

  return ctx;
}

/****
 * initializes a client context and sets up the system environment for a client
 *session
 *
 ****/
result_t shog_init_client(args_t *args) {
  client_ctx_t *ctx = new_client_ctx();

  assert(ctx != NULL);
  assert(args != NULL);

  if (args->set_config) {
    LOG(INFO, "Initializing client with custom config file: %s",
        args->config_path);
  } else {
    LOG(INFO, "Initializing client");
  }

  char default_runtime_path[FILE_PATH_SIZE];
  utils_get_default_runtime_path(default_runtime_path);

  // WARN: custom runtime path must be an absolute path, else certain dynamic
  // libraries will not load (particularly redisjson)
  if (args->set_runtime_path) {
    ctx->runtime_path = malloc((strlen(args->runtime_path) + 1) * sizeof(char));
    strcpy(ctx->runtime_path, args->runtime_path);

    LOG(INFO, "Using custom runtime path: %s", ctx->runtime_path);

    if (!utils_dir_exists(ctx->runtime_path)) {
      return ERR("custom runtime path does not exist");
    }
  } else {
    ctx->runtime_path =
        malloc((strlen(default_runtime_path) + 1) * sizeof(char));
    strcpy(ctx->runtime_path, default_runtime_path);

    LOG(INFO, "Using default runtime path: %s", ctx->runtime_path);
  }

  utils_verify_client_runtime_dirs(ctx);

  result_t res = init_client_runtime(ctx, args);
  PROPAGATE(res);

  printf("\n\n");

  return OK(ctx);
}

/****
 * frees all allocated memory and resources, then exits the process with
 * exit_code
 *
 ****/
void shoggoth_client_exit(client_ctx_t *ctx, int exit_code) {
  for (u64 i = 0; i < ctx->known_nodes->nodes_count; i++) {
    free(ctx->known_nodes->nodes[i]);
  }

  if (ctx->known_nodes->nodes_count > 0) {
    free(ctx->known_nodes->nodes);
  }

  free(ctx->manifest->public_key);
  free(ctx->manifest->shoggoth_id);
  free(ctx->manifest);

  free(ctx->runtime_path);

  free(ctx->known_nodes);

  free(ctx);

  exit(exit_code);
}

result_t client_print_id(client_ctx_t *ctx) {
  printf("Your Shoggoth ID is: %s\n", ctx->manifest->shoggoth_id);

  return OK(NULL);
}

/****
 * handles a client session.
 *
 ****/
result_t handle_client_session(args_t *args) {
  result_t res_ctx = shog_init_client(args);
  client_ctx_t *ctx = PROPAGATE(res_ctx);

  if (args->no_command_arg) {
    EXIT(1, "no argument was provided to client command");
  }

  if (strcmp(args->command_arg, "clone") == 0) {
    if (args->no_subcommand_arg) {
      EXIT(1, "no argument was provided to client `clone` subcommand");
    }

    result_t res = client_clone(ctx, args->subcommand_arg);
    PROPAGATE(res);
  } else if (strcmp(args->command_arg, "init") == 0) {
    result_t res = client_init_profile(ctx->manifest);
    PROPAGATE(res);
  } else if (strcmp(args->command_arg, "publish") == 0) {
    result_t res = client_publish_profile(ctx);
    PROPAGATE(res);
  } else if (strcmp(args->command_arg, "id") == 0) {
    client_print_id(ctx);
  } else {
    EXIT(1, "invalid client subcommand: '%s'", args->command_arg);
  }

  shoggoth_client_exit(ctx, 0);

  return OK(NULL);
}