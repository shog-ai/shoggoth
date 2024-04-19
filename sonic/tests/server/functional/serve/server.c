#include "../../../../sonic.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <netlibc.h>

void serve_200(sonic_server_request_t *req) {
  sonic_server_response_t *resp =
      sonic_new_response(STATUS_200, MIME_TEXT_PLAIN);

  char *body = "Hello world";

  sonic_response_set_body(resp, body, strlen(body));

  sonic_send_response(req, resp);
  sonic_free_server_response(resp);
}

int main() {
  NETLIBC_INIT();

  sonic_server_t *server = sonic_new_server("127.0.0.1", 5000);

  sonic_add_route(server, "/serve_200", METHOD_GET, serve_200);

  sonic_add_file_route(server, "/large_file", METHOD_GET,
                       "./large_test_file.bin", MIME_APPLICATION_OCTET_STREAM);

  // printf("Listening: http://127.0.0.1:%d/ \n", 5000);

  int failed = sonic_start_server(server);
  if (failed) {
    printf("ERROR: start server failed \n");
    exit(1);
  }

  return 0;
}
