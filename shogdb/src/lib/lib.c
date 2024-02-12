/**** lib.c ****
 *
 *  Copyright (c) 2023 ShogAI - https://shog.ai
 *
 * Part of the ShogDB database, under the MIT License.
 * See LICENSE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#include "lib.h"
#include "../../include/cjson.h"

#include <netlibc/error.h>
#include <netlibc/log.h>
#include <stdlib.h>

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

  char *msg_type = strdup(msg);
  msg_type[length] = '\0';
  // LOG(INFO, "TYPE: `%s`", msg_type);

  char *msg_value = strdup(&msg[length + 1]);
  // LOG(INFO, "VALUE: `%s`", msg_value);

  value->value_type = str_to_value_type(msg_type);
  free(msg_type);

  if (value->value_type == VALUE_STR) {
    value->value_str = msg_value;
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
