#include "../../sonic.h"

#include <stdio.h>
#include <stdlib.h>

// This example illustrates setting a custom redirect policy for a request

int main() {
  // build a request
  sonic_request_t *req =
      sonic_new_request(METHOD_GET, "https://shoggoth.systems");

  sonic_redirect_policy_t policy = {0};
  policy.max_hops = 3;

  // sets a rdirect policy that allows only 3 maximum hops
  sonic_set_redirect_policy(req, policy);

  // send the request
  sonic_response_t *resp = sonic_send_request(req);

  if (resp->failed) {
    printf("request failed: %s \n", resp->error);
    exit(1);
  } else {
    if (resp->response_body_size > 0) {
      free(resp->response_body);
    }

    // free the request
    sonic_free_request(req);

    // free the response
    sonic_free_response(resp);
  }

  // build a request
  req = sonic_new_request(METHOD_GET, "http://shoggoth.systems");

  sonic_redirect_policy_t no_redirect = {0};
  no_redirect.no_redirect = true;

  // sets a rdirect policy that disables automatic redirect handling and returns
  // the redirect response itself
  sonic_set_redirect_policy(req, no_redirect);

  // send the request
  resp = sonic_send_request(req);

  if (resp->failed) {
    printf("request failed: %s \n", resp->error);
    exit(1);
  } else {
    char *status_str = sonic_status_to_string(resp->status);
    printf("STATUS: %s \n\n", status_str);

    printf("RESPONSE BODY:\n%.*s \n", (int)resp->response_body_size,
           resp->response_body);

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