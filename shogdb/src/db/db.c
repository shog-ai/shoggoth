/**** db.c ****
 *
 *  Copyright (c) 2023 ShogAI - https://shog.ai
 *
 * Part of the ShogDB database, under the MIT License.
 * See LICENSE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#include "db.h"
#include "../../include/jansson.h"
#include "../../include/sonic.h"
#include "../hashmap/hashmap.h"
#include "dht.h"
#include "json.h"
#include "pins.h"

#include <assert.h>
#include <netlibc/fs.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include <netlibc/mem.h>

db_ctx_t *global_ctx = NULL;

char *serialize_data(db_ctx_t *ctx) {
  void *save_json = json_array();

  hashmap_t *hashmap = global_ctx->hashmap;
  hashmap_t *s;
  for (s = hashmap; s != NULL; s = s->hh.next) {
    db_value_t *value = s->value;

    void *item_json = json_object();

    json_object_set_new(item_json, "key", json_string(s->key));
    json_object_set_new(item_json, "type",
                        json_string(value_type_to_str(value->value_type)));

    if (value->value_type == VALUE_STR) {
      json_object_set_new(item_json, "value", json_string(value->value_str));
    } else if (value->value_type == VALUE_BOOL) {
      if (value->value_bool == true) {
        json_object_set_new(item_json, "value", json_string("true"));
      } else {
        json_object_set_new(item_json, "value", json_string("false"));
      }
    } else if (value->value_type == VALUE_INT) {
      char val[256];
      sprintf(val, S64_FORMAT_SPECIFIER, value->value_int);

      json_object_set_new(item_json, "value", json_string(val));
    } else if (value->value_type == VALUE_UINT) {
      char val[256];
      sprintf(val, U64_FORMAT_SPECIFIER, value->value_uint);

      json_object_set_new(item_json, "value", json_string(val));
    } else if (value->value_type == VALUE_FLOAT) {
      char val[256];
      sprintf(val, "%lf", value->value_float);

      json_object_set_new(item_json, "value", json_string(val));
    } else if (value->value_type == VALUE_JSON) {
      assert(value->value_json != NULL);
      char *val = json_to_str(value->value_json);

      json_object_set_new(item_json, "value", json_string(val));

      free(val);
    } else {
      PANIC("unhandled value type");
    }

    json_array_append_new(save_json, item_json);
  }

  char *db_data = json_to_str(save_json);
  free_json(save_json);

  return db_data;
}

db_ctx_t *new_db(db_config_t *config) {
  db_ctx_t *ctx = calloc(1, sizeof(db_ctx_t));
  ctx->hashmap = new_hashmap();
  ctx->config = config;
  ctx->should_exit = false;

  return ctx;
}

void free_db_config(db_config_t *config) {
  free(config->network.host);
  free(config->save.path);

  free(config);
}

void free_db(db_ctx_t *ctx) {
  free_db_config(ctx->config);

  free(ctx);
}

result_t db_get_value(db_ctx_t *ctx, char *key) {
  result_t res = hashmap_get(&ctx->hashmap, key);
  db_value_t *s = PROPAGATE(res);

  return OK(s);
}

result_t db_delete_value(db_ctx_t *ctx, char *key) {
  result_t res_s1 = hashmap_get(&ctx->hashmap, key);
  void *s1 = PROPAGATE(res_s1);

  hashmap_delete(&ctx->hashmap, key);

  free_db_value(s1);

  ctx->saved = false;

  return OK(NULL);
}

void db_clear_memory_data(db_ctx_t *ctx) {
  hashmap_t *hashmap = global_ctx->hashmap;
  hashmap_t *s;
  for (s = hashmap; s != NULL; s = s->hh.next) {
    db_value_t *value = s->value;

    free_db_value(value);
  }
}

void db_add_str_value(db_ctx_t *ctx, char *key, char *str) {
  db_value_t *value = new_db_value(VALUE_STR);
  value->value_str = strdup(str);

  UNWRAP(hashmap_add(&ctx->hashmap, key, value));

  ctx->saved = false;
}

void db_add_bool_value(db_ctx_t *ctx, char *key, bool val) {
  db_value_t *value = new_db_value(VALUE_BOOL);
  value->value_bool = val;

  UNWRAP(hashmap_add(&ctx->hashmap, key, value));

  ctx->saved = false;
}

void db_add_uint_value(db_ctx_t *ctx, char *key, u64 val) {
  db_value_t *value = new_db_value(VALUE_UINT);
  value->value_uint = val;

  UNWRAP(hashmap_add(&ctx->hashmap, key, value));

  ctx->saved = false;
}

void db_add_int_value(db_ctx_t *ctx, char *key, s64 val) {
  db_value_t *value = new_db_value(VALUE_INT);
  value->value_int = val;

  UNWRAP(hashmap_add(&ctx->hashmap, key, value));

  ctx->saved = false;
}

void db_add_float_value(db_ctx_t *ctx, char *key, f64 val) {
  db_value_t *value = new_db_value(VALUE_FLOAT);
  value->value_float = val;

  UNWRAP(hashmap_add(&ctx->hashmap, key, value));

  ctx->saved = false;
}

void db_add_json_value(db_ctx_t *ctx, char *key, void *val) {
  char *str = json_to_str(val);
  void *new_val = str_to_json(str);
  free(str);

  db_value_t *value = new_db_value(VALUE_JSON);
  value->value_json = new_val;

  UNWRAP(hashmap_add(&ctx->hashmap, key, value));

  ctx->saved = false;
}

result_t db_update_str_value(db_ctx_t *ctx, char *key, char *str) {
  result_t res = hashmap_get(&ctx->hashmap, key);
  db_value_t *s = PROPAGATE(res);

  if (s->value_type != VALUE_STR) {
    return ERR("value type is not a string");
  }

  pthread_mutex_lock(&s->mutex);

  s->value_str = realloc(s->value_str, (strlen(str) + 1) * sizeof(char));
  strcpy(s->value_str, str);

  pthread_mutex_unlock(&s->mutex);

  ctx->saved = false;

  return OK(NULL);
}

result_t db_update_bool_value(db_ctx_t *ctx, char *key, bool val) {
  result_t res = hashmap_get(&ctx->hashmap, key);
  db_value_t *s = PROPAGATE(res);

  if (s->value_type != VALUE_BOOL) {
    return ERR("value type is not bool");
  }

  pthread_mutex_lock(&s->mutex);

  s->value_bool = val;

  pthread_mutex_unlock(&s->mutex);

  ctx->saved = false;

  return OK(NULL);
}

result_t db_update_uint_value(db_ctx_t *ctx, char *key, u64 val) {
  result_t res = hashmap_get(&ctx->hashmap, key);
  db_value_t *s = PROPAGATE(res);

  if (s->value_type != VALUE_UINT) {
    return ERR("value type is not uint");
  }

  pthread_mutex_lock(&s->mutex);

  s->value_uint = val;

  pthread_mutex_unlock(&s->mutex);

  ctx->saved = false;

  return OK(NULL);
}

result_t db_update_int_value(db_ctx_t *ctx, char *key, s64 val) {
  result_t res = hashmap_get(&ctx->hashmap, key);
  db_value_t *s = PROPAGATE(res);

  if (s->value_type != VALUE_INT) {
    return ERR("value type is not int");
  }

  pthread_mutex_lock(&s->mutex);

  s->value_int = val;

  pthread_mutex_unlock(&s->mutex);

  ctx->saved = false;

  return OK(NULL);
}

result_t db_update_float_value(db_ctx_t *ctx, char *key, f64 val) {
  result_t res = hashmap_get(&ctx->hashmap, key);
  db_value_t *s = PROPAGATE(res);

  if (s->value_type != VALUE_FLOAT) {
    return ERR("value type is not float");
  }

  pthread_mutex_lock(&s->mutex);

  s->value_float = val;

  pthread_mutex_unlock(&s->mutex);

  ctx->saved = false;

  return OK(NULL);
}

void home_route(sonic_server_request_t *req) {
  sonic_server_response_t *resp =
      sonic_new_response(STATUS_200, MIME_TEXT_PLAIN);

  char *body = "Hello world";
  sonic_response_set_body(resp, body, strlen(body));

  sonic_send_response(req, resp);

  sonic_free_server_response(resp);
}

void get_route(sonic_server_request_t *req) {
  char *key = sonic_get_path_segment(req, "key");

  result_t res = db_get_value(global_ctx, key);

  if (is_ok(res)) {
    db_value_t *value = VALUE(res);

    char *body = shogdb_print_value(value);

    sonic_server_response_t *resp =
        sonic_new_response(STATUS_200, MIME_TEXT_PLAIN);
    sonic_response_set_body(resp, body, strlen(body));
    sonic_send_response(req, resp);

    free(body);

    sonic_free_server_response(resp);

  } else {
    sonic_server_response_t *resp =
        sonic_new_response(STATUS_406, MIME_TEXT_PLAIN);

    char err[256];
    sprintf(err, "ERR %s", res.error_message);

    sonic_response_set_body(resp, err, strlen(err));
    sonic_send_response(req, resp);
    sonic_free_server_response(resp);
  }
}

void set_route(sonic_server_request_t *req) {
  char *key = sonic_get_path_segment(req, "key");

  result_t res_existing = db_get_value(global_ctx, key);

  if (is_ok(res_existing)) {
    sonic_server_response_t *resp =
        sonic_new_response(STATUS_406, MIME_TEXT_PLAIN);

    char err[256];
    sprintf(err, "ERR key already exists");

    sonic_response_set_body(resp, err, strlen(err));
    sonic_send_response(req, resp);

    sonic_free_server_response(resp);

    free_result(res_existing);

    return;
  } else {
    free_result(res_existing);
  }

  if (req->request_body_size < 5) {
    sonic_server_response_t *resp =
        sonic_new_response(STATUS_406, MIME_TEXT_PLAIN);

    char err[256];
    sprintf(err, "ERR request body too smol");

    sonic_response_set_body(resp, err, strlen(err));
    sonic_send_response(req, resp);

    sonic_free_server_response(resp);
    return;
  }

  char *req_body = malloc((req->request_body_size + 1) * sizeof(char));
  strncpy(req_body, req->request_body, req->request_body_size);
  req_body[req->request_body_size] = '\0';

  result_t res_value = shogdb_parse_message(req_body);
  if (is_err(res_value)) {
    sonic_server_response_t *resp =
        sonic_new_response(STATUS_406, MIME_TEXT_PLAIN);

    char err[256];
    sprintf(err, "ERR %s", res_value.error_message);

    sonic_response_set_body(resp, err, strlen(err));
    sonic_send_response(req, resp);

    sonic_free_server_response(resp);
    return;
  }

  db_value_t *value = VALUE(res_value);

  if (value->value_type == VALUE_STR) {
    db_add_str_value(global_ctx, key, value->value_str);
  } else if (value->value_type == VALUE_BOOL) {
    db_add_bool_value(global_ctx, key, value->value_bool);
  } else if (value->value_type == VALUE_UINT) {
    db_add_uint_value(global_ctx, key, value->value_uint);
  } else if (value->value_type == VALUE_INT) {
    db_add_int_value(global_ctx, key, value->value_int);
  } else if (value->value_type == VALUE_FLOAT) {
    db_add_float_value(global_ctx, key, value->value_float);
  } else if (value->value_type == VALUE_JSON) {
    db_add_json_value(global_ctx, key, value->value_json);
  } else if (value->value_type == VALUE_NULL) {
    sonic_server_response_t *resp =
        sonic_new_response(STATUS_406, MIME_TEXT_PLAIN);

    char err[256];
    sprintf(err, "ERR invalid value type");

    sonic_response_set_body(resp, err, strlen(err));
    sonic_send_response(req, resp);

    sonic_free_server_response(resp);
    return;
  } else {
    sonic_server_response_t *resp =
        sonic_new_response(STATUS_406, MIME_TEXT_PLAIN);

    char err[256];
    sprintf(err, "ERR unhandled value type");

    sonic_response_set_body(resp, err, strlen(err));
    sonic_send_response(req, resp);

    sonic_free_server_response(resp);
    return;
  }

  sonic_server_response_t *resp =
      sonic_new_response(STATUS_200, MIME_TEXT_PLAIN);

  char *body = "OK";
  sonic_response_set_body(resp, body, strlen(body));

  sonic_send_response(req, resp);

  free(req_body);
  free_db_value(value);
  sonic_free_server_response(resp);
}

void delete_route(sonic_server_request_t *req) {
  char *key = sonic_get_path_segment(req, "key");

  result_t res = db_delete_value(global_ctx, key);
  if (is_err(res)) {
    sonic_server_response_t *resp =
        sonic_new_response(STATUS_406, MIME_TEXT_PLAIN);

    char err[256];
    sprintf(err, "ERR %s", res.error_message);

    sonic_response_set_body(resp, err, strlen(err));
    sonic_send_response(req, resp);

    sonic_free_server_response(resp);
    return;
  }

  free_result(res);

  sonic_server_response_t *resp =
      sonic_new_response(STATUS_200, MIME_TEXT_PLAIN);

  char *body = "OK";
  sonic_response_set_body(resp, body, strlen(body));

  sonic_send_response(req, resp);

  sonic_free_server_response(resp);
}

void db_exit(int exit_code) {
  if (global_ctx->server_started) {
    sonic_stop_server(global_ctx->http_server);
  }

  global_ctx->should_exit = true;

  sleep(2);

  if (global_ctx->server_started) {
    free(global_ctx->http_server);
  }

  db_save_data(global_ctx);

  db_clear_memory_data(global_ctx);

  exit(exit_code);
}

void exit_handler(int sig) {
  if (sig == SIGINT || sig == SIGTERM) {
    LOG(INFO, "STOPPING DB ...");
  } else if (sig == SIGSEGV) {
    printf("\nSEGMENTATION FAULT");
    fflush(stdout);
  }

  if (sig == SIGINT || sig == SIGTERM) {
    db_exit(0);
  } else if (sig == SIGSEGV) {
    db_exit(1);
  }
}

void set_signal_handlers() {
  // Set the SIGINT (CTRL-C) handler to the custom handler
  if (signal(SIGINT, exit_handler) == SIG_ERR) {
    PANIC("could not set SIGINT signal handler");
  }

  // Register the signal handler for SIGTERM
  if (signal(SIGTERM, exit_handler) == SIG_ERR) {
    PANIC("could not set SIGTERM signal handler");
  }
}

void print_route(sonic_server_request_t *req) {
  char *db_data = serialize_data(global_ctx);

  sonic_server_response_t *resp =
      sonic_new_response(STATUS_200, MIME_TEXT_PLAIN);

  char *body = db_data;
  sonic_response_set_body(resp, body, strlen(body));

  sonic_send_response(req, resp);

  free(db_data);
  sonic_free_server_response(resp);
}

void db_save_data(db_ctx_t *ctx) {
  if (ctx->saved) {
    return;
  }

  LOG(INFO, "saving data to %s", ctx->config->save.path);

  char *db_data = serialize_data(ctx);

  UNWRAP(write_to_file(ctx->config->save.path, db_data, strlen(db_data)));

  free(db_data);

  ctx->saved = true;
}

result_t db_restore_data(db_ctx_t *ctx) {
  if (!file_exists(ctx->config->save.path)) {
    return OK_INT(1);
  }

  LOG(INFO, "restoring data from %s", ctx->config->save.path);

  char *data = UNWRAP(read_file_to_string(ctx->config->save.path));
  void *data_json = str_to_json(data);
  free(data);

  void *item_json = NULL;
  size_t index;
  json_array_foreach(data_json, index, item_json) {
    char *key = (char *)json_string_value(json_object_get(item_json, "key"));
    char *type_str =
        (char *)json_string_value(json_object_get(item_json, "type"));
    char *value_str =
        (char *)json_string_value(json_object_get(item_json, "value"));

    value_type_t value_type = str_to_value_type(type_str);

    if (value_type == VALUE_STR) {
      db_add_str_value(ctx, key, value_str);
    } else if (value_type == VALUE_UINT) {
      u64 result = 0;

      if (sscanf(value_str, U64_FORMAT_SPECIFIER, &result) != 1) {
        return ERR("sscanf conversion failed");
      }

      db_add_uint_value(ctx, key, result);
    } else if (value_type == VALUE_INT) {
      s64 result = 0;

      if (sscanf(value_str, S64_FORMAT_SPECIFIER, &result) != 1) {
        return ERR("sscanf conversion failed");
      }

      db_add_int_value(ctx, key, result);
    } else if (value_type == VALUE_FLOAT) {
      f64 result = 0;

      if (sscanf(value_str, "%lf", &result) != 1) {
        return ERR("sscanf conversion failed");
      }

      db_add_float_value(ctx, key, result);
    } else if (value_type == VALUE_BOOL) {
      if (strcmp(value_str, "true") == 0) {
        db_add_bool_value(ctx, key, true);
      } else if (strcmp(value_str, "false") == 0) {
        db_add_bool_value(ctx, key, false);
      } else {
        PANIC("invalid boolean value");
      }
    } else if (value_type == VALUE_JSON) {
      void *result = str_to_json(value_str);

      db_add_json_value(ctx, key, result);

      free_json(result);
    } else {
      PANIC("unhandled value type");
    }
  }

  free_json(data_json);

  return OK_INT(0);
}

sonic_server_t *create_server(db_ctx_t *ctx) {
  sonic_server_t *server =
      sonic_new_server(ctx->config->network.host, ctx->config->network.port);

  sonic_add_route(server, "/", METHOD_GET, home_route);
  sonic_add_route(server, "/get/{key}", METHOD_GET, get_route);
  sonic_add_route(server, "/set/{key}", METHOD_GET, set_route);
  sonic_add_route(server, "/delete/{key}", METHOD_GET, delete_route);
  sonic_add_route(server, "/print/", METHOD_GET, print_route);

  add_dht_routes(server);

  add_pins_routes(server);

  add_json_routes(ctx, server);

  return server;
}

void *start_data_saver(void *args) {
  args = (void *)args;

  for (;;) {
    if (global_ctx->should_exit) {
      return NULL;
    }

    sleep(global_ctx->config->save.interval);

    db_save_data(global_ctx);

    if (global_ctx->should_exit) {
      return NULL;
    }
  }

  return NULL;
}

result_t start_db(db_ctx_t *ctx) {
  global_ctx = ctx;

  set_signal_handlers();

  sonic_server_t *server = create_server(ctx);
  ctx->http_server = server;

  LOG(INFO, "Database running at http://%s:%d", ctx->config->network.host,
      ctx->config->network.port);

  pthread_t data_saver_thread;

  if (ctx->config->save.enabled) {
    int restored = UNWRAP_INT(db_restore_data(ctx));

    if (restored == 1) {
      LOG(INFO, "Setting up DHT and PINS");
      UNWRAP(setup_dht(ctx));
      UNWRAP(setup_pins(ctx));
    }

    if (pthread_create(&data_saver_thread, NULL, start_data_saver, NULL) != 0) {
      PANIC("could not spawn server thread");
    }
  }

  ctx->server_started = true;
  int failed = sonic_start_server(server);
  if (failed) {
    printf("ERROR: start server failed \n");
    exit(1);
  }

  if (ctx->config->save.enabled) {
    pthread_join(data_saver_thread, NULL);
  }

  return OK(NULL);
}
