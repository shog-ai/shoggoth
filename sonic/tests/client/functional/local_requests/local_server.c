#include "../../../../sonic.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void home_route(sonic_server_request_t *req) {
  sonic_server_response_t *resp =
      sonic_new_response(STATUS_200, MIME_TEXT_PLAIN);

  char *body = "Hello world";

  sonic_response_set_body(resp, body, strlen(body));

  sonic_send_response(req, resp);
  sonic_free_server_response(resp);
}

void post_with_body(sonic_server_request_t *req) {
  printf("REQUEST BODY:%.*s", (int)req->request_body_size, req->request_body);

  fflush(stdout);

  sonic_server_response_t *resp =
      sonic_new_response(STATUS_200, MIME_TEXT_PLAIN);

  char *body = "DONE";

  sonic_response_set_body(resp, body, strlen(body));

  sonic_send_response(req, resp);
  sonic_free_server_response(resp);
}

void post_with_headers(sonic_server_request_t *req) {
  char *header1 =
      sonic_get_header_value(req->headers, req->headers_count, "My-Header1");

  char *header2 =
      sonic_get_header_value(req->headers, req->headers_count, "My-Header2");

  printf("HEADER1:%s|HEADER2:%s", header1, header2);

  fflush(stdout);

  sonic_server_response_t *resp =
      sonic_new_response(STATUS_200, MIME_TEXT_PLAIN);

  char *body = "DONE";

  sonic_response_set_body(resp, body, strlen(body));

  sonic_send_response(req, resp);
  sonic_free_server_response(resp);
}

int main() {
  sonic_server_t *server = sonic_new_server("127.0.0.1", 5000);

  sonic_add_route(server, "/", METHOD_GET, home_route);
  sonic_add_route(server, "/post_with_body", METHOD_POST, post_with_body);
  sonic_add_route(server, "/post_with_headers", METHOD_POST, post_with_headers);

  sonic_add_file_route(server, "/response_callback", METHOD_GET,
                       "./large_test_file.bin", MIME_APPLICATION_OCTET_STREAM);

  // printf("Listening: http://127.0.0.1:%d/ \n", 5000);

  int failed = sonic_start_server(server);
  if (failed) {
    printf("ERROR: start server failed \n");
    exit(1);
  }

  return 0;
}
