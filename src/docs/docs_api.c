#include <stdlib.h>

#include "../include/cjson.h"
#include "../json/json.h"
#include "../md_to_html/md_to_html.h"
#include "../templating/templating.h"

typedef struct {
  char *key;
  char *value;
} endpoint_header_t;

typedef struct {
  char *status;
  char *description;
  char *body;

  endpoint_header_t *headers;
  u64 headers_count;
} endpoint_response_t;

typedef struct {
  char *endpoint;
  char *description;
  char *request_body;

  endpoint_header_t *headers;
  u64 headers_count;

  endpoint_response_t **responses;
  u64 responses_count;
} api_endpoint_t;

typedef struct {
  api_endpoint_t **items;
  u64 items_count;
} api_endpoints_t;

json_t *endpoint_response_to_json(endpoint_response_t item) {
  cJSON *response_json = cJSON_CreateObject();

  cJSON_AddStringToObject(response_json, "status", item.status);

  cJSON_AddStringToObject(response_json, "description", item.description);

  cJSON_AddStringToObject(response_json, "body", item.body);

  cJSON *headers_json = cJSON_CreateArray();

  for (u64 k = 0; k < item.headers_count; k++) {
    cJSON *header_json = cJSON_CreateObject();

    cJSON_AddStringToObject(header_json, "key", item.headers[k].key);
    cJSON_AddStringToObject(header_json, "value", item.headers[k].value);

    cJSON_AddItemToArray(headers_json, header_json);
  }

  cJSON_AddItemToObject(response_json, "headers", headers_json);

  return (void *)response_json;
}

json_t *api_endpoint_to_json(api_endpoint_t item) {
  cJSON *item_json = cJSON_CreateObject();

  cJSON_AddStringToObject(item_json, "endpoint", item.endpoint);

  cJSON_AddStringToObject(item_json, "description", item.description);

  cJSON_AddStringToObject(item_json, "request_body", item.request_body);

  cJSON *responses_json = cJSON_CreateArray();

  for (u64 k = 0; k < item.responses_count; k++) {
    cJSON *response_json =
        (cJSON *)endpoint_response_to_json(*item.responses[k]);

    cJSON_AddItemToArray(responses_json, response_json);
  }

  cJSON_AddItemToObject(item_json, "responses", responses_json);

  cJSON *headers_json = cJSON_CreateArray();

  for (u64 k = 0; k < item.headers_count; k++) {
    cJSON *header_json = cJSON_CreateObject();

    cJSON_AddStringToObject(header_json, "key", item.headers[k].key);
    cJSON_AddStringToObject(header_json, "value", item.headers[k].value);

    cJSON_AddItemToArray(headers_json, header_json);
  }

  cJSON_AddItemToObject(item_json, "headers", headers_json);

  return (void *)item_json;
}

json_t *api_endpoints_to_json(api_endpoints_t *items) {
  cJSON *items_json = cJSON_CreateArray();

  for (u64 i = 0; i < items->items_count; i++) {
    cJSON *item_json = (cJSON *)api_endpoint_to_json(*items->items[i]);

    cJSON_AddItemToArray(items_json, item_json);
  }

  return (void *)items_json;
}

api_endpoints_t *new_api_endpoints() {
  api_endpoints_t *items = malloc(sizeof(api_endpoints_t *));

  items->items = NULL;
  items->items_count = 0;

  return items;
}

api_endpoint_t *new_api_endpoint(char *endpoint, char *description,
                                 char *request_body) {
  api_endpoint_t *item = calloc(1, sizeof(api_endpoint_t));

  item->endpoint = strdup(endpoint);
  item->description = strdup(description);
  item->request_body = strdup(request_body);

  item->headers = NULL;
  item->headers_count = 0;

  item->responses = NULL;
  item->responses_count = 0;

  return item;
}

endpoint_response_t *new_endpoint_response(char *status, char *description,
                                           char *body) {
  endpoint_response_t *response = malloc(sizeof(endpoint_response_t));

  response->status = strdup(status);
  response->description = strdup(description);
  response->body = strdup(body);

  response->headers = NULL;
  response->headers_count = 0;

  return response;
}

void endpoint_add_header(api_endpoint_t *item, char *key, char *value) {
  item->headers = realloc(item->headers, (item->headers_count + 1) *
                                             sizeof(endpoint_header_t));

  item->headers[item->headers_count] =
      (endpoint_header_t){.key = strdup(key), .value = strdup(value)};

  item->headers_count++;
}

void endpoint_response_add_header(endpoint_response_t *response, char *key,
                                  char *value) {
  response->headers = realloc(response->headers, (response->headers_count + 1) *
                                                     sizeof(endpoint_header_t));

  response->headers[response->headers_count] =
      (endpoint_header_t){.key = strdup(key), .value = strdup(value)};

  response->headers_count++;
}

void free_endpoint_response(endpoint_response_t *response) {
  free(response->status);
  free(response->description);
  free(response->body);

  if (response->headers_count > 0) {
    for (u64 i = 0; i < response->headers_count; i++) {
      free(response->headers[i].key);
      free(response->headers[i].value);
    }

    free(response->headers);
  }

  free(response);
}

void free_api_endpoint(api_endpoint_t *item) {
  free(item->endpoint);
  free(item->description);
  free(item->request_body);

  if (item->headers_count > 0) {
    for (u64 i = 0; i < item->headers_count; i++) {
      free(item->headers[i].key);
      free(item->headers[i].value);
    }

    free(item->headers);
  }

  if (item->responses_count > 0) {
    for (u64 i = 0; i < item->responses_count; i++) {
      free_endpoint_response(item->responses[i]);
    }

    free(item->responses);
  }

  free(item);
}

void free_api_endpoints(api_endpoints_t *items) {
  for (u64 i = 0; i < items->items_count; i++) {
    free_api_endpoint(items->items[i]);
  }

  free(items);
}

void api_endpoints_add_endpoint(api_endpoints_t *items, api_endpoint_t *item) {
  items->items = realloc(items->items,
                         (items->items_count + 1) * sizeof(api_endpoint_t *));

  items->items[items->items_count] = item;

  items->items_count++;
}

void api_endpoint_add_response(api_endpoint_t *item,
                               endpoint_response_t *response) {
  item->responses = realloc(item->responses, (item->responses_count + 1) *
                                                 sizeof(endpoint_response_t *));

  item->responses[item->responses_count] = response;

  item->responses_count++;
}

void add_all_api_endpoints(api_endpoints_t *api_items);

result_t gen_api_docs() {
  api_endpoints_t *api_endpoints = new_api_endpoints();

  add_all_api_endpoints(api_endpoints);

  json_t *endpoints_json = api_endpoints_to_json(api_endpoints);
  char *endpoints_str = json_to_string(endpoints_json);
  free_api_endpoints(api_endpoints);

  char *api_template_data =
      malloc((strlen(endpoints_str) + 100) * sizeof(char));
  sprintf(api_template_data,
          "{ \"apis\": %s , \"count\": " U64_FORMAT_SPECIFIER "}",
          endpoints_str, api_endpoints->items_count);
  free(endpoints_str);

  result_t res_end_template_string =
      read_file_to_string("./explorer/templates/end.html");
  char *end_template_string = UNWRAP(res_end_template_string);

  template_t *end_template = create_template(end_template_string, "{}");
  free(end_template_string);

  result_t res_head_template_string =
      read_file_to_string("./explorer/templates/head.html");
  char *head_template_string = UNWRAP(res_head_template_string);

  template_t *head_template = create_template(
      head_template_string,
      "{\"title\": \"Shoggoth API Reference\", \"desc\": \"Shoggoth is a "
      "peer-to-peer, anonymous network for publishing and distributing "
      "open-source code, Machine Learning models, datasets, and research "
      "papers.\", \"og_url\": \"/explorer/static/img/og/docs_api.png\", "
      "\"url\": \"/explorer/docs/api\", \"is_api\": true}");
  free(head_template_string);

  result_t res_api_template_string =
      read_file_to_string("./explorer/docs/md/api.md");
  char *api_template_string = UNWRAP(res_api_template_string);

  template_t *api_template =
      create_template(api_template_string, api_template_data);
  free(api_template_string);
  free(api_template_data);

  template_add_partial(api_template, "head", head_template);
  template_add_partial(api_template, "end", end_template);

  result_t res_cooked_api = cook_template(api_template);
  char *cooked_api = UNWRAP(res_cooked_api);

  free_template(api_template);
  free_template(head_template);
  free_template(end_template);

  write_to_file("./explorer/out/docs/api.html", cooked_api, strlen(cooked_api));

  // LOG(INFO, "COOKED: %s", cooked_api);

  free(cooked_api);

  md_file_to_html_file("./explorer/out/docs/api.html",
                       "./explorer/out/docs/api.html");

  return OK(NULL);
}

void download_endpoint(api_endpoints_t *endpoints, char *url) {
  api_endpoint_t *endpoint =
      new_api_endpoint(url, "download a shoggoth resource", "NONE");
  // endpoint_add_header(endpoint, "deezkey", "deezvalue");

  endpoint_response_t *response1 =
      new_endpoint_response("200 OK", "a response containing the resource",
                            "a binary blob containing the resource");
  api_endpoint_add_response(endpoint, response1);

  endpoint_response_t *response2 =
      new_endpoint_response("302 Found",
                            "The resource is not pinned by the current node, "
                            "but a peer has pinned it so redirect to the peer.",
                            "NONE");
  endpoint_response_add_header(response2, "Location",
                               "URL of peer that has pinned the resource");
  api_endpoint_add_response(endpoint, response2);

  endpoint_response_t *response3 = new_endpoint_response(
      "406 Not Acceptable",
      "a response indicating that the resource was not found", "NONE");
  api_endpoint_add_response(endpoint, response3);

  api_endpoints_add_endpoint(endpoints, endpoint);
}

void get_dht_endpoint(api_endpoints_t *endpoints) {
  api_endpoint_t *endpoint =
      new_api_endpoint("GET /api/get_dht", "Get the DHT of the node", "NONE");

  endpoint_response_t *response1 = new_endpoint_response(
      "200 OK", "response containing the DHT", "the DHT as a JSON string");
  api_endpoint_add_response(endpoint, response1);

  endpoint_response_t *response3 = new_endpoint_response(
      "406 Not Acceptable", "an error occured", "the error");
  api_endpoint_add_response(endpoint, response3);

  api_endpoints_add_endpoint(endpoints, endpoint);
}

void get_manifest_endpoint(api_endpoints_t *endpoints) {
  api_endpoint_t *endpoint = new_api_endpoint(
      "GET /api/get_manifest", "Get the manifest of the node", "NONE");

  endpoint_response_t *response1 =
      new_endpoint_response("200 OK", "response containing the manifest",
                            "the manifest as a JSON string");
  api_endpoint_add_response(endpoint, response1);

  endpoint_response_t *response3 = new_endpoint_response(
      "406 Not Acceptable", "an error occured", "the error");
  api_endpoint_add_response(endpoint, response3);

  api_endpoints_add_endpoint(endpoints, endpoint);
}

void get_pins_endpoint(api_endpoints_t *endpoints) {
  api_endpoint_t *endpoint = new_api_endpoint(
      "GET /api/get_pins", "Get a list of all the pins of the node", "NONE");

  endpoint_response_t *response1 =
      new_endpoint_response("200 OK", "response containing the pins",
                            "the array of pins as a JSON string");
  api_endpoint_add_response(endpoint, response1);

  endpoint_response_t *response3 = new_endpoint_response(
      "406 Not Acceptable", "an error occured", "the error");
  api_endpoint_add_response(endpoint, response3);

  api_endpoints_add_endpoint(endpoints, endpoint);
}

void add_all_api_endpoints(api_endpoints_t *endpoints) {
  download_endpoint(endpoints, "GET /api/download/{shoggoth_id}");

  get_dht_endpoint(endpoints);
  get_manifest_endpoint(endpoints);
  get_pins_endpoint(endpoints);
}
