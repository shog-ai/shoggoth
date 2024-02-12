#include "../../../../sonic.h"

#include <stdlib.h>

int main(int argc, char **argv) {
  sonic_request_t *req =
      sonic_new_request(METHOD_GET, "http://127.0.0.1:5000/redirect1");

  sonic_redirect_policy_t policy = {0};
  policy.max_hops = 3;

  sonic_set_redirect_policy(req, policy);

  sonic_response_t *resp = sonic_send_request(req);

  if (resp->failed) {
    if (strcmp(resp->error, "Maximum redirect hops of 3 reached") == 0) {
      printf("MAX HOPS");
      fflush(stdout);
    } else {
      printf("request failed: %s \n", resp->error);
      exit(1);
    }
  }

  free(resp->response_body);

  sonic_free_request(req);
  sonic_free_response(resp);
}
