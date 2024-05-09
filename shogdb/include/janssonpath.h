#ifndef JANSSONPATH
#define JANSSONPATH

#include <jansson.h>

// for simple jsonpath like $.a.b[0] we got a non-collection
// but for $[*] $[1:12], etc. we got a collection
// note distinct shuld be made dealing with intermidiate result
// so use path_result instead of json_t*
typedef struct path_result {
    json_t *result;
    int is_collection;
} path_result;

json_t *json_path_get(json_t *json, const char *path);
path_result json_path_get_distinct(json_t *json, const char *path);

#endif