#include "../../sonic.h"

#include <stdio.h>
#include <stdlib.h>

// This example illustrates sending a request with a body and handling a
// response body

void response_callback(char *data, u64 size, void *user_pointer) {
  // printed to make the chunks clearly visible and individual
  printf("\n\n\n\nCALLBACK!!\n\n\n\n");

  printf("DATA:\n %.*s\n", (int)size, data);
}

int main() {
  // construct a new request
  // this request illustrates setting a body for a post request
  sonic_request_t *req =
      sonic_new_request(METHOD_POST, "https://shog.ai");

  char *request_body = "Hello world!";
  sonic_set_body(req, request_body, strlen(request_body));

  // send the request
  sonic_response_t *resp = sonic_send_request(req);

  if (resp->failed) {
    printf("request failed: %s \n", resp->error);
    exit(1);
  } else {
    // free the request
    sonic_free_request(req);

    // free the response
    sonic_free_response(resp);
  }

  // construct a new request
  // this request illustrates printing the body of a response
  req = sonic_new_request(METHOD_GET, "https://shog.ai");

  // send the request
  resp = sonic_send_request(req);

  if (resp->failed) {
    printf("request failed: %s \n", resp->error);
    exit(1);
  } else {
    // note that response body is not a null terminated string
    printf("RESPONSE BODY:\n%.*s \n", (int)resp->response_body_size,
           resp->response_body);

    // always manually free the response body if the size is greater than 0
    if (resp->response_body_size > 0) {
      free(resp->response_body);
    }

    // free the request
    sonic_free_request(req);

    // free the response
    sonic_free_response(resp);
  }

  // construct a new request
  // this request illustrates setting a custom callback function to capture the
  // response body
  req = sonic_new_request(METHOD_GET, "https://shog.ai");

  // set a callback function to handle the response body
  // Note that if this function is used on a request, nothing will be written to
  // the response_body and no memory is allocated for the response body.
  // instead, the body will be sent to the callback function in chunks.
  // This is useful when the response body may be very large like when
  // downloading large files.
  sonic_request_set_response_callback(req, response_callback, NULL);

  // send the request
  resp = sonic_send_request(req);

  if (resp->failed) {
    printf("request failed: %s \n", resp->error);
    exit(1);
  } else {
    // there is no need to free the resp->response_body because a response callback
    // was set

    // free the request
    sonic_free_request(req);

    // free the response
    sonic_free_response(resp);
  }

  return 0;
}
