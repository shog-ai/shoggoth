
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

sonic_middleware_response_t *middleware_function(sonic_server_request_t *req,
                                                 void *user_pointer) {
  printf("MIDDLEWARE CALLED \n");

  // sonic_server_response_t *resp =
  // sonic_new_response(STATUS_200, MIME_TEXT_PLAIN);

  // char *body = "Hello from the middleware";
  // sonic_response_set_body(resp, body, strlen(body));

  sonic_middleware_response_t *response =
      sonic_new_middleware_response(false, NULL);

  return response;
}

sonic_middleware_response_t *
server_middleware_function(sonic_server_request_t *req, void *user_pointer) {
  printf("SERVER MIDDLEWARE CALLED \n");

  sonic_middleware_response_t *response =
      sonic_new_middleware_response(false, NULL);

  return response;
}

int main() {
  sonic_server_t *server = sonic_new_server("0.0.0.0", 3000);
  sonic_add_server_middleware(server, server_middleware_function, NULL);
  sonic_add_server_rate_limiter_middleware(
      server, 3, 5000, "you are being rate limited. try again later");

  sonic_route_t *route1 = sonic_add_route(server, "/1", METHOD_GET, home_route);
  sonic_add_middleware(route1, middleware_function, NULL);

  sonic_route_t *route2 = sonic_add_route(server, "/2", METHOD_GET, home_route);

  printf("Listening: http://127.0.0.1:%d/ \n", 3000);

  // start the server
  int failed = sonic_start_server(server);
  if (failed) {
    printf("ERROR: start server failed \n");
    exit(1);
  }

  return 0;
}
