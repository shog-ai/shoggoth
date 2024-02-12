/**** hashmap.h ****
 *
 *  Copyright (c) 2023 ShogAI - https://shog.ai
 *
 * Part of the ShogDB database, under the MIT License.
 * See LICENSE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#ifndef SHOGDB_HASHMAP_H
#define SHOGDB_HASHMAP_H

#include "uthash.h"
#include <netlibc/error.h>

typedef struct {
  char *key; /* we'll use this field as the key */
  void *value;
  UT_hash_handle hh; /* makes this structure hashable */
} hashmap_t;

hashmap_t *new_hashmap();

result_t hashmap_add(hashmap_t **hashmap, char *hashmap_key, void *value);

result_t hashmap_get(hashmap_t **hashmap, char *key);

hashmap_t *hashmap_find(hashmap_t **hashmap, char *key);

result_t hashmap_delete(hashmap_t **hashmap, char *key);

#endif
