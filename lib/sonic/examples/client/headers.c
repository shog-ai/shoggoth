#include "../../sonic.h"

#include <stdio.h>
#include <stdlib.h>

// This example illustrates working with request headers and response headers

int main() {
  // construct a request
  sonic_request_t *req =
      sonic_new_request(METHOD_GET, "https://shoggoth.systems");

  // add a custom header
  sonic_add_header(req, "MyCustomHeader", "Header Value");

  // send the request
  sonic_response_t *resp = sonic_send_request(req);

  if (resp->failed) {
    printf("request failed: %s \n", resp->error);
    exit(1);
  } else {
    // get the value of a header
    char *content_type = sonic_get_header_value(
        resp->headers, resp->headers_count, "Content-Type");

    if (content_type != NULL) {
      printf("Content-Type header value -> %s \n\n", content_type);
    } else {
      printf("No Content-Type header was found \n");
      exit(1);
    }

    printf("All Response Headers -> \n\n");

    // enumerate and print all headers
    for (int i = 0; i < resp->headers_count; i++) {
      printf("%s: %s \n", resp->headers[i].key, resp->headers[i].value);
    }

    if (resp->response_body_size > 0) {
      free(resp->response_body);
    }

    // free the request
    sonic_free_request(req);

    // free the response
    sonic_free_response(resp);
  }

  return 0;
}