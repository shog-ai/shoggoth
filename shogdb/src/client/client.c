/**** lib.c ****
 *
 *  Copyright (c) 2023 ShogAI - https://shog.ai
 *
 * Part of the ShogDB database, under the MIT License.
 * See LICENSE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#include "client.h"
#include "../../include/cjson.h"
#include "../../include/sonic.h"

#include <netlibc/error.h>
#include <netlibc/log.h>
#include <netlibc/mem.h>
#include <netlibc/string.h>
#include <stdlib.h>

char *shogdb_print_value(db_value_t *value) {
  char *value_type = value_type_to_str(value->value_type);

  char *res = NULL;

  if (value->value_type == VALUE_STR) {
    res = string_from(value_type, " ", value->value_str, NULL);
  } else if (value->value_type == VALUE_ERR) {
    res = string_from(value_type, " ", value->value_err, NULL);
  } else if (value->value_type == VALUE_BOOL) {
    if (value->value_bool == 1) {
      res = string_from(value_type, " true", NULL);
    } else {
      res = string_from(value_type, " false", NULL);
    }
  } else if (value->value_type == VALUE_UINT) {
    char val[256];
    sprintf(val, U64_FORMAT_SPECIFIER, value->value_uint);

    res = string_from(value_type, " ", val, NULL);
  } else if (value->value_type == VALUE_INT) {
    char val[256];
    sprintf(val, S64_FORMAT_SPECIFIER, value->value_int);

    res = string_from(value_type, " ", val, NULL);
  } else if (value->value_type == VALUE_FLOAT) {
    char val[256];
    sprintf(val, F64_FORMAT_SPECIFIER, value->value_float);

    res = string_from(value_type, " ", val, NULL);
  } else if (value->value_type == VALUE_JSON) {
    char *str = cJSON_Print(value->value_json);
    res = string_from(value_type, " ", str, NULL);
    free(str);
  } else {
    PANIC("unhandled type");
  }

  return res;
}

result_t shogdb_set(shogdb_ctx_t *ctx, char *key, char *value) {
  char url[256];
  sprintf(url, "%s/set/%s", ctx->address, key);

  sonic_request_t *req = sonic_new_request(METHOD_GET, url);

  sonic_set_body(req, value, strlen(value));

  sonic_response_t *resp = sonic_send_request(req);
  sonic_free_request(req);

  if (resp->failed) {
    return ERR("request failed: %s \n", resp->error);
  }

  if (strcmp(resp->response_body, "OK") != 0) {
    char *msg = nstrdup(resp->response_body);

    return ERR(msg);
  } else {
    nfree(resp->response_body);
    sonic_free_response(resp);

    return OK(NULL);
  }
}

result_t shogdb_json_append(shogdb_ctx_t *ctx, char *key, char *filter,
                            char *value) {
  char url[256];
  sprintf(url, "%s/json_append/%s/%s", ctx->address, key, filter);

  sonic_request_t *req = sonic_new_request(METHOD_GET, url);

  sonic_set_body(req, value, strlen(value));

  sonic_response_t *resp = sonic_send_request(req);
  sonic_free_request(req);

  if (resp->failed) {
    return ERR("request failed: %s \n", resp->error);
  }

  if (strcmp(resp->response_body, "OK") != 0) {
    char *msg = nstrdup(resp->response_body);

    return ERR(msg);
  } else {
    nfree(resp->response_body);
    sonic_free_response(resp);

    return OK(NULL);
  }
}

result_t shogdb_set_int(shogdb_ctx_t *ctx, char *key, s64 value) {
  char body[256];
  sprintf(body, "INT " S64_FORMAT_SPECIFIER, value);

  result_t res = shogdb_set(ctx, key, body);

  return res;
}

result_t shogdb_set_uint(shogdb_ctx_t *ctx, char *key, u64 value) {
  char body[256];
  sprintf(body, "UINT " U64_FORMAT_SPECIFIER, value);

  result_t res = shogdb_set(ctx, key, body);

  return res;
}

result_t shogdb_set_float(shogdb_ctx_t *ctx, char *key, f64 value) {
  char body[256];
  sprintf(body, "FLOAT " F64_FORMAT_SPECIFIER, value);

  result_t res = shogdb_set(ctx, key, body);

  return res;
}

result_t shogdb_set_str(shogdb_ctx_t *ctx, char *key, char *value) {
  char *body = string_from("STR ", value, NULL);

  result_t res = shogdb_set(ctx, key, body);

  nfree(body);

  return res;
}

result_t shogdb_set_bool(shogdb_ctx_t *ctx, char *key, bool value) {
  char body[256];
  if (value == true) {
    sprintf(body, "BOOL true");
  } else {
    sprintf(body, "BOOL false");
  }

  result_t res = shogdb_set(ctx, key, body);

  return res;
}

result_t shogdb_set_json(shogdb_ctx_t *ctx, char *key, char *value) {
  char *body = string_from("JSON ", value, NULL);

  result_t res = shogdb_set(ctx, key, body);

  nfree(body);

  return res;
}

result_t shogdb_get(shogdb_ctx_t *ctx, char *key) {
  char url[256];
  sprintf(url, "%s/get/%s", ctx->address, key);

  sonic_request_t *req = sonic_new_request(METHOD_GET, url);

  sonic_response_t *resp = sonic_send_request(req);
  sonic_free_request(req);

  if (resp->failed) {
    return ERR("request failed: %s \n", resp->error);
  }

  result_t res = shogdb_parse_message(resp->response_body);
  db_value_t *value = PROPAGATE(res);

  if (value->value_type == VALUE_ERR) {
    return ERR(value->value_err);
  }

  nfree(resp->response_body);
  sonic_free_response(resp);

  return OK(value);
}

result_t shogdb_print(shogdb_ctx_t *ctx) {
  char url[256];
  sprintf(url, "%s/print", ctx->address);

  sonic_request_t *req = sonic_new_request(METHOD_GET, url);

  sonic_response_t *resp = sonic_send_request(req);
  sonic_free_request(req);

  if (resp->failed) {
    return ERR("request failed: %s \n", resp->error);
  }

  char *res = nstrdup(resp->response_body);

  nfree(resp->response_body);
  sonic_free_response(resp);

  return OK(res);
}

result_t shogdb_delete(shogdb_ctx_t *ctx, char *key) {
  char url[256];
  sprintf(url, "%s/delete/%s", ctx->address, key);

  sonic_request_t *req = sonic_new_request(METHOD_GET, url);

  sonic_response_t *resp = sonic_send_request(req);
  sonic_free_request(req);

  if (resp->failed) {
    return ERR("request failed: %s \n", resp->error);
  }

  nfree(resp->response_body);
  sonic_free_response(resp);

  return OK(NULL);
}

shogdb_ctx_t *new_shogdb(char *address) {
  shogdb_ctx_t *ctx = ncalloc(1, sizeof(shogdb_ctx_t));
  ctx->address = nstrdup(address);

  return ctx;
}

void free_shogdb(shogdb_ctx_t *ctx) {
  nfree(ctx->address);

  nfree(ctx);
}

db_value_t *new_db_value(value_type_t value_type) {
  db_value_t *db_value = calloc(1, sizeof(db_value_t));
  db_value->value_type = value_type;

  db_value->value_str = NULL;
  db_value->value_json = NULL;

  return db_value;
}

value_type_t str_to_value_type(char *str) {
  if (strcmp(str, "STR") == 0) {
    return VALUE_STR;
  } else if (strcmp(str, "ERR") == 0) {
    return VALUE_ERR;
  } else if (strcmp(str, "BOOL") == 0) {
    return VALUE_BOOL;
  } else if (strcmp(str, "UINT") == 0) {
    return VALUE_UINT;
  } else if (strcmp(str, "INT") == 0) {
    return VALUE_INT;
  } else if (strcmp(str, "FLOAT") == 0) {
    return VALUE_FLOAT;
  } else if (strcmp(str, "JSON") == 0) {
    return VALUE_JSON;
  } else {
    return VALUE_NULL;
  }
}

char *value_type_to_str(value_type_t value_type) {
  if (value_type == VALUE_STR) {
    return "STR";
  } else if (value_type == VALUE_ERR) {
    return "ERR";
  } else if (value_type == VALUE_BOOL) {
    return "BOOL";
  } else if (value_type == VALUE_UINT) {
    return "UINT";
  } else if (value_type == VALUE_INT) {
    return "INT";
  } else if (value_type == VALUE_FLOAT) {
    return "FLOAT";
  } else if (value_type == VALUE_JSON) {
    return "JSON";
  } else if (value_type == VALUE_NULL) {
    return "NULL";
  } else {
    PANIC("unhandled value type");
    return "NULL";
  }
}

void free_db_value(db_value_t *value) {
  if (value->value_type == VALUE_STR) {
    free(value->value_str);
  }

  if (value->value_type == VALUE_ERR) {
    free(value->value_err);
  }

  if (value->value_type == VALUE_JSON) {
    cJSON_Delete(value->value_json);
  }

  pthread_mutex_destroy(&value->mutex);

  free(value);
}

result_t shogdb_parse_message(char *msg) {
  if (strchr(msg, ' ') == NULL) {
    return ERR("The string does not contain a space");
  }

  db_value_t *value = new_db_value(VALUE_STR);

  size_t length = strcspn(msg, " ");

  char *msg_type = nstrdup(msg);
  msg_type[length] = '\0';
  // LOG(INFO, "TYPE: `%s`", msg_type);

  char *msg_value = nstrdup(&msg[length + 1]);
  // LOG(INFO, "VALUE: `%s`", msg_value);

  value->value_type = str_to_value_type(msg_type);
  nfree(msg_type);

  if (value->value_type == VALUE_STR) {
    value->value_str = nstrdup(msg_value);
  } else if (value->value_type == VALUE_ERR) {
    value->value_err = nstrdup(msg_value);
  } else if (value->value_type == VALUE_BOOL) {
    bool result = false;

    if (strcmp(msg_value, "true") == 0) {
      result = true;
    } else if (strcmp(msg_value, "false") == 0) {
      result = false;
    } else {
      return ERR("invalid boolean value");
    }

    value->value_bool = result;
  } else if (value->value_type == VALUE_UINT) {
    u64 result = 0;

    if (sscanf(msg_value, U64_FORMAT_SPECIFIER, &result) != 1) {
      return ERR("sscanf conversion failed");
    }

    value->value_uint = result;
  } else if (value->value_type == VALUE_INT) {
    s64 result = 0;

    if (sscanf(msg_value, S64_FORMAT_SPECIFIER, &result) != 1) {
      return ERR("sscanf conversion failed");
    }

    value->value_int = result;
  } else if (value->value_type == VALUE_FLOAT) {
    f64 result = 0.0;

    if (sscanf(msg_value, "%lf", &result) != 1) {
      return ERR("sscanf conversion failed");
    }

    value->value_float = result;
  } else if (value->value_type == VALUE_JSON) {
    value->value_json = cJSON_Parse(msg_value);

    if (value->value_json == NULL) {
      return ERR("could not parse JSON");
    }
  } else if (value->value_type == VALUE_NULL) {
    return ERR("invalid value type");
  } else {
    return ERR("unhandled value type");
  }

  free(msg_value);

  return OK(value);
}
