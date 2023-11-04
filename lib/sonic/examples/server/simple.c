#include "../../sonic.h"

#include <stdio.h>
#include <stdlib.h>

// This example illustrates creating a simple Sonic HTTP server

void home_route(sonic_server_request_t *req) {
  // create a response
  sonic_server_response_t *resp =
      sonic_new_response(STATUS_200, MIME_TEXT_PLAIN);

  char *body = "Hello world";
  sonic_response_set_body(resp, body, strlen(body));

  // send the response
  sonic_send_response(req, resp);

  // free the response
  sonic_free_server_response(resp);
}

int main() {
  // create a server
  sonic_server_t *server = sonic_new_server("127.0.0.1", 5000);

  // add a route
  sonic_add_route(server, "/", METHOD_GET, home_route);

  printf("Listening: http://127.0.0.1:%d/ \n", 5000);

  // start the server
  int failed = sonic_start_server(server);
  if (failed) {
    printf("ERROR: start server failed \n");
    exit(1);
  }

  return 0;
}
