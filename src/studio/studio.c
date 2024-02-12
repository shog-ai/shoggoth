/**** studio.c ****
 *
 *  Copyright (c) 2023 ShogAI
 *
 * Part of the Shoggoth project, under the MIT License.
 * See LICENSE file for license information.
 * SPDX-License-Identifier: MIT
 *
 ****/

#include "../args/args.h"
#include "../include/sonic.h"

void index_route(sonic_server_request_t *req) {
  char *body = "Hello world";

  sonic_server_response_t *resp =
      sonic_new_response(STATUS_200, MIME_TEXT_PLAIN);
  sonic_response_set_body(resp, body, strlen(body));

  sonic_response_add_header(resp, "Access-Control-Allow-Origin", "*");

  sonic_send_response(req, resp);
  sonic_free_server_response(resp);
}

void add_frontend_routes(sonic_server_t *server) {
  sonic_add_route(server, "/", METHOD_GET, index_route);
}

/****
 * starts a http server exposing the Shog Studio frontend
 *
 ****/
void *start_studio_server() {
  sonic_server_t *server = sonic_new_server("127.0.0.1", 6968);

  add_frontend_routes(server);

  int failed = sonic_start_server(server);
  if (failed) {
    LOG(ERROR, "start studio server failed");
  }

  LOG(INFO, "studio server exited");

  return NULL;
}

result_t start_studio(args_t *args) {
  args = (void *)args;

  LOG(INFO, "Starting Shog Studio at https://127.0.0.1:6968");

  start_studio_server();

  return OK(NULL);
}
