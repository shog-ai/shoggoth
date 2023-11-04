#include "../../sonic.h"

#include <stdio.h>
#include <stdlib.h>

// This example illustrates creating a simple Sonic HTTP server with generic
// path segments in a route

void hello_route(sonic_server_request_t *req) {
  // create a response
  sonic_server_response_t *resp =
      sonic_new_response(STATUS_200, MIME_TEXT_PLAIN);

  char *body = "Hi world";

  sonic_response_set_body(resp, body, strlen(body));

  // send the response
  sonic_send_response(req, resp);

  // free the response
  sonic_free_server_response(resp);
}

void greet_route(sonic_server_request_t *req) {
  char *first_name = sonic_get_path_segment(req, "first_name");
  char *last_name = sonic_get_path_segment(req, "last_name");

  // create a response
  sonic_server_response_t *resp =
      sonic_new_response(STATUS_200, MIME_TEXT_PLAIN);

  char body[256];
  sprintf(body, "Hello %s %s", first_name, last_name);

  // char *body = "Hello world";

  sonic_response_set_body(resp, body, strlen(body));

  // send the response
  sonic_send_response(req, resp);

  // free the response
  sonic_free_server_response(resp);
}

void greet_wildcard(sonic_server_request_t *req) {
  sonic_wildcard_segments_t wildcard_segments =
      sonic_get_path_wildcard_segments(req);

  // create a response
  sonic_server_response_t *resp =
      sonic_new_response(STATUS_200, MIME_TEXT_PLAIN);

  char body[512];
  sprintf(body, "Hello ");

  for (u64 i = 0; i < wildcard_segments.count; i++) {
    strcat(body, wildcard_segments.segments[i]);

    strcat(body, " and ");
  }

  sonic_free_path_wildcard_segments(wildcard_segments);

  // char *body = "Hello world";

  sonic_response_set_body(resp, body, strlen(body));

  // send the response
  sonic_send_response(req, resp);

  // free the response
  sonic_free_server_response(resp);
}

int main() {
  // create a server
  sonic_server_t *server = sonic_new_server("127.0.0.1", 5000);

  sonic_add_route(server, "/greet/everyone/{*}/", METHOD_GET, greet_wildcard);

  // add a route
  sonic_add_route(server, "/greet/{first_name}/{last_name}", METHOD_GET,
                  greet_route);

  sonic_add_route(server, "/", METHOD_GET, hello_route);

  sonic_add_route(server, "/hello/", METHOD_GET, hello_route);

  printf("Listening: http://127.0.0.1:%d/ \n", 5000);

  // start the server
  int failed = sonic_start_server(server);
  if (failed) {
    printf("ERROR: start server failed \n");
    exit(1);
  }

  return 0;
}
