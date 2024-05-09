/**** pins.c ****
 *
 *  Copyright (c) 2023 ShogAI - https://shog.ai
 *
 * Part of the ShogDB database, under the MIT License.
 * See LICENSE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#include "pins.h"
#include "../../include/sonic.h"
#include "db.h"

db_ctx_t *pins_ctx = NULL;

// void pins_remove_resource_route(sonic_server_request_t *req) {
//   char *shoggoth_id = sonic_get_path_segment(req, "shoggoth_id");

//   result_t res = db_get_value(pins_ctx, "pins");

//   if (is_ok(res)) {
//     db_value_t *value = VALUE(res);
//     cJSON *pins = value->value_json;

//     int index = 0;

//     const cJSON *pin_json = NULL;
//     cJSON_ArrayForEach(pin_json, pins) {

//       char *pin_shoggoth_id =
//           cJSON_GetObjectItemCaseSensitive(pin_json, "shoggoth_id")
//               ->valuestring;

//       if (strcmp(shoggoth_id, pin_shoggoth_id) == 0) {
//         cJSON_DeleteItemFromArray(pins, index);

//         sonic_server_response_t *resp =
//             sonic_new_response(STATUS_200, MIME_TEXT_PLAIN);

//         char *body = "OK";
//         sonic_response_set_body(resp, body, strlen(body));
//         sonic_send_response(req, resp);

//         sonic_free_server_response(resp);

//         pins_ctx->saved = false;

//         return;
//       }

//       index++;
//     }

//     sonic_server_response_t *resp =
//         sonic_new_response(STATUS_406, MIME_TEXT_PLAIN);

//     char err[256];
//     sprintf(err, "ERR pin with shoggoth_id not found");

//     sonic_response_set_body(resp, err, strlen(err));
//     sonic_send_response(req, resp);
//     sonic_free_server_response(resp);
//   } else {
//     sonic_server_response_t *resp =
//         sonic_new_response(STATUS_406, MIME_TEXT_PLAIN);

//     char err[256];
//     sprintf(err, "ERR %s", res.error_message);

//     sonic_response_set_body(resp, err, strlen(err));
//     sonic_send_response(req, resp);
//     sonic_free_server_response(resp);
//   }
// }

// void get_pin_label_route(sonic_server_request_t *req) {
//   char *shoggoth_id = sonic_get_path_segment(req, "shoggoth_id");

//   result_t res = db_get_value(pins_ctx, "pins");

//   if (is_ok(res)) {
//     db_value_t *value = VALUE(res);
//     cJSON *pins = value->value_json;

//     const cJSON *pin_json = NULL;
//     cJSON_ArrayForEach(pin_json, pins) {

//       char *pin_shoggoth_id =
//           cJSON_GetObjectItemCaseSensitive(pin_json, "shoggoth_id")
//               ->valuestring;

//       if (strcmp(shoggoth_id, pin_shoggoth_id) == 0) {
//         char *label =
//             cJSON_GetObjectItemCaseSensitive(pin_json, "label")->valuestring;

//         sonic_server_response_t *resp =
//             sonic_new_response(STATUS_200, MIME_TEXT_PLAIN);

//         char *body = label;
//         sonic_response_set_body(resp, body, strlen(body));
//         sonic_send_response(req, resp);

//         sonic_free_server_response(resp);

//         return;
//       }
//     }

//     sonic_server_response_t *resp =
//         sonic_new_response(STATUS_406, MIME_TEXT_PLAIN);

//     char err[256];
//     sprintf(err, "ERR pin with shoggoth_id not found");

//     sonic_response_set_body(resp, err, strlen(err));
//     sonic_send_response(req, resp);
//     sonic_free_server_response(resp);
//   } else {
//     sonic_server_response_t *resp =
//         sonic_new_response(STATUS_406, MIME_TEXT_PLAIN);

//     char err[256];
//     sprintf(err, "ERR %s", res.error_message);

//     sonic_response_set_body(resp, err, strlen(err));
//     sonic_send_response(req, resp);
//     sonic_free_server_response(resp);
//   }
// }

// void pins_clear_route(sonic_server_request_t *req) {
//   result_t res = db_get_value(pins_ctx, "pins");

//   if (is_ok(res)) {
//     db_value_t *value = VALUE(res);

//     cJSON_Delete(value->value_json);

//     value->value_json = cJSON_CreateArray();

//     sonic_server_response_t *resp =
//         sonic_new_response(STATUS_200, MIME_TEXT_PLAIN);

//     char *body = "OK";
//     sonic_response_set_body(resp, body, strlen(body));
//     sonic_send_response(req, resp);

//     sonic_free_server_response(resp);

//     pins_ctx->saved = false;

//     return;
//   } else {
//     sonic_server_response_t *resp =
//         sonic_new_response(STATUS_406, MIME_TEXT_PLAIN);

//     char err[256];
//     sprintf(err, "ERR %s", res.error_message);

//     sonic_response_set_body(resp, err, strlen(err));
//     sonic_send_response(req, resp);
//     sonic_free_server_response(resp);
//   }
// }

// void pins_add_resource_route(sonic_server_request_t *req) {
//   char *shoggoth_id = sonic_get_path_segment(req, "shoggoth_id");
//   char *label = sonic_get_path_segment(req, "label");

//   result_t res = db_get_value(pins_ctx, "pins");

//   if (is_ok(res)) {
//     db_value_t *value = VALUE(res);

//     cJSON *pin_json = cJSON_CreateObject();

//     cJSON_AddStringToObject(pin_json, "shoggoth_id", shoggoth_id);
//     cJSON_AddStringToObject(pin_json, "label", label);

//     cJSON_AddItemToArray(value->value_json, pin_json);

//     sonic_server_response_t *resp =
//         sonic_new_response(STATUS_200, MIME_TEXT_PLAIN);

//     char *body = "OK";
//     sonic_response_set_body(resp, body, strlen(body));
//     sonic_send_response(req, resp);

//     sonic_free_server_response(resp);

//     pins_ctx->saved = false;

//     return;
//   } else {
//     sonic_server_response_t *resp =
//         sonic_new_response(STATUS_406, MIME_TEXT_PLAIN);

//     char err[256];
//     sprintf(err, "ERR %s", res.error_message);

//     sonic_response_set_body(resp, err, strlen(err));
//     sonic_send_response(req, resp);
//     sonic_free_server_response(resp);

//     return;
//   }
// }

// void get_pins_route(sonic_server_request_t *req) {
//   result_t res = db_get_value(pins_ctx, "pins");

//   if (is_ok(res)) {
//     db_value_t *value = VALUE(res);

//     if (value->value_type == VALUE_JSON) {
//       char *str = cJSON_Print(value->value_json);

//       char *body = malloc((strlen(str) + 10) * sizeof(char));
//       sprintf(body, "%s %s", value_type_to_str(VALUE_JSON), str);

//       sonic_server_response_t *resp =
//           sonic_new_response(STATUS_200, MIME_TEXT_PLAIN);
//       sonic_response_set_body(resp, body, strlen(body));
//       sonic_send_response(req, resp);

//       free(str);
//       free(body);

//       sonic_free_server_response(resp);
//     } else {
//       sonic_server_response_t *resp =
//           sonic_new_response(STATUS_406, MIME_TEXT_PLAIN);

//       char err[256];
//       sprintf(err, "ERR pins is not a JSON type");

//       sonic_response_set_body(resp, err, strlen(err));
//       sonic_send_response(req, resp);
//       sonic_free_server_response(resp);
//     }
//   } else {
//     sonic_server_response_t *resp =
//         sonic_new_response(STATUS_406, MIME_TEXT_PLAIN);

//     char err[256];
//     sprintf(err, "ERR %s", res.error_message);

//     sonic_response_set_body(resp, err, strlen(err));
//     sonic_send_response(req, resp);
//     sonic_free_server_response(resp);
//   }
// }

void add_pins_routes(sonic_server_t *server) {
  // sonic_add_route(server, "/pins/get_pins", METHOD_GET, get_pins_route);
  // sonic_add_route(server, "/pins/get_pin_label/{shoggoth_id}", METHOD_GET,
  //                 get_pin_label_route);
  // sonic_add_route(server, "/pins/add_resource/{shoggoth_id}/{label}",
  //                 METHOD_GET, pins_add_resource_route);
  // sonic_add_route(server, "/pins/remove_resource", METHOD_GET,
  //                 pins_remove_resource_route);
  // sonic_add_route(server, "/pins/clear", METHOD_GET, pins_clear_route);
}

result_t setup_pins(db_ctx_t *ctx) {
  pins_ctx = ctx;

  // cJSON *value_json = cJSON_CreateArray();

  // db_add_json_value(ctx, "pins", value_json);

  // cJSON_Delete(value_json);

  return OK(NULL);
}
