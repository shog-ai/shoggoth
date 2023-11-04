#include "../../../../sonic.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void redirect1(sonic_server_request_t *req) {
  sonic_server_response_t *resp =
      sonic_new_response(STATUS_302, MIME_TEXT_PLAIN);

  sonic_response_add_header(resp, "Location",
                            "http://127.0.0.1:5000/redirect2");

  char *body = "REDIRECT1";

  sonic_response_set_body(resp, body, strlen(body));

  sonic_send_response(req, resp);
  sonic_free_server_response(resp);
}

void redirect2(sonic_server_request_t *req) {
  sonic_server_response_t *resp =
      sonic_new_response(STATUS_302, MIME_TEXT_PLAIN);

  sonic_response_add_header(resp, "Location",
                            "http://127.0.0.1:5000/redirect3");

  char *body = "REDIRECT2";

  sonic_response_set_body(resp, body, strlen(body));

  sonic_send_response(req, resp);
  sonic_free_server_response(resp);
}

void redirect3(sonic_server_request_t *req) {
  sonic_server_response_t *resp =
      sonic_new_response(STATUS_302, MIME_TEXT_PLAIN);

  sonic_response_add_header(resp, "Location",
                            "http://127.0.0.1:5000/redirect4");

  char *body = "REDIRECT3";

  sonic_response_set_body(resp, body, strlen(body));

  sonic_send_response(req, resp);
  sonic_free_server_response(resp);
}

void redirect4(sonic_server_request_t *req) {
  sonic_server_response_t *resp =
      sonic_new_response(STATUS_302, MIME_TEXT_PLAIN);

  sonic_response_add_header(resp, "Location",
                            "http://127.0.0.1:5000/redirect5");

  char *body = "REDIRECT4";

  sonic_response_set_body(resp, body, strlen(body));

  sonic_send_response(req, resp);
  sonic_free_server_response(resp);
}

void redirect5(sonic_server_request_t *req) {
  sonic_server_response_t *resp =
      sonic_new_response(STATUS_200, MIME_TEXT_PLAIN);

  char *body = "REDIRECT5";

  sonic_response_set_body(resp, body, strlen(body));

  sonic_send_response(req, resp);
  sonic_free_server_response(resp);
}

int main() {
  sonic_server_t *server = sonic_new_server("127.0.0.1", 5000);

  sonic_add_route(server, "/redirect1", METHOD_GET, redirect1);
  sonic_add_route(server, "/redirect2", METHOD_GET, redirect2);
  sonic_add_route(server, "/redirect3", METHOD_GET, redirect3);
  sonic_add_route(server, "/redirect4", METHOD_GET, redirect4);
  sonic_add_route(server, "/redirect5", METHOD_GET, redirect5);

  // printf("Listening: http://127.0.0.1:%d/ \n", 5000);

  int failed = sonic_start_server(server);
  if (failed) {
    printf("ERROR: start server failed \n");
    exit(1);
  }

  return 0;
}
