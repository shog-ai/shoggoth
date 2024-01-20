#include "../../sonic.h"

#include <stdio.h>
#include <stdlib.h>

// This example illustrates the simple semantics of using a Sonic client to
// fetch a HTTP resource

int main() {
  // using simple get function
  sonic_response_t *resp = sonic_get("http://localhost:5000");

  if (resp->failed) {
    printf("request failed: %s \n", resp->error);
    exit(1);
  } else {
    // note that response body is not a null terminated string
    printf("RESPONSE BODY:\n%.*s \n", (int)resp->response_body_size,
           resp->response_body);

    // always free the response body if the size is greater than 0
    if (resp->response_body_size > 0) {
      free(resp->response_body);
    }

    // frees the response
    // response body must be freed manually as shown above
    sonic_free_response(resp);
  }

  // building a request with custom method
  sonic_request_t *req =
      sonic_new_request(METHOD_GET, "https://shog.ai");
  resp = sonic_send_request(req);

  if (resp->failed) {
    printf("request failed: %s \n", resp->error);
    exit(1);
  } else {
    // note that response body is not a null terminated string
    printf("RESPONSE BODY:\n%.*s \n", (int)resp->response_body_size,
           resp->response_body);

    // always free the response body if the size is greater than 0
    if (resp->response_body_size > 0) {
      free(resp->response_body);
    }

    // frees the request
    sonic_free_request(req);

    // frees the response
    // response body must be freed manually as shown above
    sonic_free_response(resp);
  }

  return 0;
}