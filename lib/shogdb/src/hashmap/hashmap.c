/**** hashmap.c ****
 *
 *  Copyright (c) 2023 ShogAI - https://shog.ai
 *
 * Part of the ShogDB database, under the MIT License.
 * See LICENCE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#include "hashmap.h"
#include "uthash.h"

#include <assert.h>
#include <netlibc/error.h>

hashmap_t *new_hashmap() { return NULL; }

result_t hashmap_add(hashmap_t **hashmap, char *hashmap_key, void *value) {
  if (value == NULL) {
    result_t err = ERR("value is NULL");
    err.error_code = 1;
    return err;
  }

  hashmap_t *s = calloc(1, sizeof(hashmap_t));
  s->key = strdup(hashmap_key);
  s->value = value;

  HASH_ADD_STR(*hashmap, key, s);

  return OK(NULL);
}

hashmap_t *hashmap_find(hashmap_t **hashmap, char *key) {
  hashmap_t *s;

  HASH_FIND_STR(*hashmap, key, s);

  return s;
}

result_t hashmap_get(hashmap_t **hashmap, char *key) {
  hashmap_t *s = hashmap_find(hashmap, key);
  if (s == NULL) {
    result_t err = ERR("key not found");
    err.error_code = 2;
    return err;
  }

  if (s->value == NULL) {
    result_t err = ERR("key's value is NULL");
    err.error_code = 3;
    return err;
  }

  return OK(s->value);
}

result_t hashmap_delete(hashmap_t **hashmap, char *key) {
  hashmap_t *s = hashmap_find(hashmap, key);
  if (s == NULL) {
    result_t err = ERR("key not found");
    err.error_code = 1;
    return err;
  }

  HASH_DEL(*hashmap, s);

  free(s->key);

  free(s);

  return OK(NULL);
}
