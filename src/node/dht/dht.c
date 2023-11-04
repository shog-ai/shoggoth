/**** dht.c ****
 *
 *  Copyright (c) 2023 Shoggoth Systems
 *
 * Part of the Shoggoth project, under the MIT License.
 * See LICENCE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#include "../../json/json.h"
#include "../db/db.h"
#include "../manifest/manifest.h"

#include <assert.h>
#include <stdlib.h>

/****
 * constructs a new dht_t
 *
 ****/
dht_t *new_dht() {
  dht_t *dht = malloc(sizeof(dht_t));

  dht->items = NULL;
  dht->items_count = 0;

  return dht;
}

void free_dht_item(dht_item_t *item) {
  free(item->host);
  free(item->node_id);
  free(item->public_key);

  free_pins(item->pins);

  free(item);
}

void free_dht(dht_t *dht) {
  if (dht->items_count > 0) {
    for (u64 i = 0; i < dht->items_count; i++) {
      free_dht_item(dht->items[i]);
    }

    free(dht->items);
  }

  free(dht);
}

/****
 * constructs a new dht_item_t
 *
 ****/
dht_item_t *new_dht_item() {
  dht_item_t *item = calloc(1, sizeof(dht_item_t));

  item->host = NULL;

  item->node_id = NULL;
  item->public_key = NULL;
  item->unreachable_count = 0;

  item->pins = calloc(1, sizeof(pins_t));
  item->pins->pins = NULL;
  item->pins->pins_count = 0;

  return item;
}

/****U
 * adds a dht item to a dht
 *
 ****/
void dht_add_item(dht_t *dht, dht_item_t *item) {
  if (dht->items_count == 0) {
    dht->items = malloc(sizeof(dht_item_t *));
  } else {
    dht->items =
        realloc(dht->items, (dht->items_count + 1) * sizeof(dht_item_t *));
  }

  dht->items[dht->items_count] = item;

  dht->items_count++;
}

/****U
 * converts a dht item to an allocated string
 *
 ****/
result_t dht_item_to_str(dht_item_t *item) {
  result_t res_item_json = json_dht_item_to_json(*item);
  json_t *item_json = PROPAGATE(res_item_json);

  char *item_str = json_to_string(item_json);

  free_json(item_json);

  return OK(item_str);
}

bool is_ip_external(const char *ip_address) {
  // Define the private IP address ranges
  const char *private_ranges[] = {"10.", "172.16.", "192.168.", "127."};

  // Compare the IP address with private ranges
  for (size_t i = 0; i < sizeof(private_ranges) / sizeof(private_ranges[0]);
       i++) {
    if (strncmp(ip_address, private_ranges[i], strlen(private_ranges[i])) ==
        0) {
      return false; // It's a private IP address
    }
  }

  // If none of the private ranges match, consider it an external IP
  return true;
}

bool valid_peer_host(node_ctx_t *ctx, char *peer_host) {
  if (strlen(peer_host) < 10 || (strncmp(peer_host, "http://", 7) != 0 &&
                                 strncmp(peer_host, "https://", 8) != 0)) {
    return false;
  }

  char *host = NULL;
  if (strncmp(peer_host, "http://", 7) == 0) {
    host = &peer_host[7];
  } else if (strncmp(peer_host, "https://", 8) == 0) {
    host = &peer_host[8];
  }

  if (!ctx->config->network.allow_private_network) {
    bool is_external = is_ip_external(host);
    if (!is_external) {
      return false;
    }
  }

  return true;
}

/****
 * adds a new peer to the dht
 *
 ****/
result_t add_new_peer(node_ctx_t *ctx, char *peer_host) {
  if (!valid_peer_host(ctx, peer_host)) {
    return ERR("peer host not valid");
  }

  char req_url[256];
  sprintf(req_url, "%s/api/get_manifest", peer_host);

  sonic_request_t *req = sonic_new_request(METHOD_GET, req_url);
  if (req == NULL) {
    PANIC(sonic_get_error());
  }

  sonic_response_t *resp = sonic_send_request(req);
  if (resp->failed) {
    sonic_free_request(req);

    char err[256];
    strcpy(err, resp->error);
    sonic_free_response(resp);

    return ERR("could not get manifest when adding peer: %s", err);
  }

  char *manifest_str = NULL;

  if (resp->response_body_size > 0) {
    char *resp_body_str = malloc((resp->response_body_size + 1) * sizeof(char));
    strncpy(resp_body_str, resp->response_body, resp->response_body_size);

    resp_body_str[resp->response_body_size] = '\0';

    manifest_str = resp_body_str;
  }

  sonic_free_request(req);
  sonic_free_response(resp);

  if (manifest_str == NULL) {
    return ERR("no response in get manifest request");
  }

  result_t res_manifest = json_string_to_node_manifest(manifest_str);
  node_manifest_t *manifest = PROPAGATE(res_manifest);

  if (!valid_peer_host(ctx, manifest->public_host)) {
    free_node_manifest(manifest);
    return ERR("peer manifest public_host not valid");
  }

  result_t res_dht_str = db_get_dht_str(ctx);
  if (is_err(res_dht_str)) {
    free_node_manifest(manifest);
    return ERR("could not get dht");
  }
  char *dht_str = VALUE(res_dht_str);

  result_t res_dht = json_string_to_dht(dht_str);
  if (is_err(res_dht)) {
    free(dht_str);
    free_node_manifest(manifest);
    return ERR("could not parse dht");
  }
  dht_t *dht = VALUE(res_dht);

  free(dht_str);

  bool exists = false;

  for (u64 i = 0; i < dht->items_count; i++) {
    if (strcmp(manifest->node_id, dht->items[i]->node_id) == 0) {
      exists = true;
    }
  }

  free_dht(dht);

  if (!exists) {
    LOG(INFO, "NEW PEER: %s", manifest->node_id);

    dht_item_t *item = new_dht_item();
    item->host = strdup(manifest->public_host);
    item->node_id = strdup(manifest->node_id);
    item->public_key = strdup(manifest->public_key);

    db_dht_add_item(ctx, item);

    free_dht_item(item);
  }

  free_node_manifest(manifest);

  return OK(NULL);
}

/****
 * starts an infinite loop that periodically updates the dht. This function runs
 * in a separate new thread
 *
 ****/
void *dht_updater(void *thread_arg) {
  dht_thread_args_t *arg = (dht_thread_args_t *)thread_arg;

  if (!arg->ctx->config->dht.enable_updater) {
    LOG(WARN, "DHT updater disabled");

    return NULL;
  }

  for (;;) {
    if (arg->ctx->should_exit) {

      return NULL;
    }

    sleep(arg->ctx->config->dht.updater_frequency);

    if (arg->ctx->should_exit) {

      return NULL;
    }

    result_t res_dht_str = db_get_dht_str(arg->ctx);
    char *dht_str = UNWRAP(res_dht_str);

    result_t res_dht = json_string_to_dht(dht_str);
    dht_t *dht = UNWRAP(res_dht);

    free(dht_str);

    for (u64 i = 0; i < dht->items_count; i++) {
      char req_url[FILE_PATH_SIZE];
      sprintf(req_url, "%s/api/get_dht", dht->items[i]->host);

      sonic_request_t *req = sonic_new_request(METHOD_GET, req_url);
      if (req == NULL) {
        PANIC(sonic_get_error());
      }

      result_t res_local_manifest_json =
          json_node_manifest_to_json(*arg->ctx->manifest);
      json_t *local_manifest_json = UNWRAP(res_local_manifest_json);
      char *local_manifest_str = json_to_string(local_manifest_json);
      free_json(local_manifest_json);

      sonic_set_body(req, local_manifest_str, strlen(local_manifest_str));

      sonic_response_t *resp = sonic_send_request(req);
      if (resp->failed) {
        LOG(WARN, "could not get remote dht of %s from %s: %s",
            dht->items[i]->node_id, dht->items[i]->host, resp->error);

        db_increment_unreachable_count(arg->ctx, dht->items[i]->node_id);

        result_t res_unreachable_count =
            db_get_unreachable_count(arg->ctx, dht->items[i]->node_id);
        char *unreachable_count_str = UNWRAP(res_unreachable_count);
        assert(strlen(unreachable_count_str) > 2);

        char *count_str = &unreachable_count_str[1];
        count_str[strlen(count_str) - 1] = '\0';

        u64 count = strtoull(count_str, NULL, 10);
        free(unreachable_count_str);

        u64 count_limit = 5;

        if (count >= count_limit) {
          LOG(INFO, "REMOVING PEER %s", dht->items[i]->node_id);
          db_dht_remove_item(arg->ctx, dht->items[i]->node_id);
        }

        sonic_free_request(req);
        sonic_free_response(resp);

        continue;
      }

      db_reset_unreachable_count(arg->ctx, dht->items[i]->node_id);

      char *response_body_str =
          malloc((resp->response_body_size + 1) * sizeof(char));

      if (resp->failed || resp->response_body == NULL) {
        LOG(WARN,
            "could not get remote dht - request failed || response_body == "
            "NULL: %s",
            resp->error);
        sonic_free_request(req);
        sonic_free_response(resp);
        continue;
      }

      strncpy(response_body_str, resp->response_body, resp->response_body_size);

      response_body_str[resp->response_body_size] = '\0';

      free(resp->response_body);

      sonic_free_request(req);
      sonic_free_response(resp);

      result_t res_remote_dht = json_string_to_dht(response_body_str);
      if (is_err(res_remote_dht)) {
        LOG(WARN, "could not parse remote dht");

        continue;
      }
      dht_t *remote_dht = VALUE(res_remote_dht);

      free(response_body_str);

      for (u64 k = 0; k < remote_dht->items_count; k++) {
        char *remote_node_id = remote_dht->items[k]->node_id;
        char *remote_host = remote_dht->items[k]->host;

        bool item_exists = false;

        for (u64 w = 0; w < dht->items_count; w++) {
          char *exists_node_id = dht->items[w]->node_id;

          if (strcmp(exists_node_id, remote_node_id) == 0) {
            item_exists = true;
          }
        }

        if (!item_exists &&
            !(strcmp(remote_node_id, arg->ctx->manifest->node_id) == 0)) {

          result_t res = add_new_peer(arg->ctx, remote_host);
          free_result(res);
        } else {
        }
      }

      free_dht(remote_dht);
    }

    free_dht(dht);
  }

  return NULL;
}
