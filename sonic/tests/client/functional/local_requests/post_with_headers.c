#include "../../../../sonic.h"

#include <stdlib.h>

int main(int argc, char **argv) {
  sonic_request_t *req =
      sonic_new_request(METHOD_POST, "http://127.0.0.1:5000/post_with_headers");

  sonic_add_header(req, "My-Header1", "value1");
  sonic_add_header(req, "My-Header2", "value2");

  sonic_response_t *resp = sonic_send_request(req);

  if (resp->failed) {
    printf("request failed: %s \n", resp->error);

    exit(1);
  }

  fflush(stdout);

  free(resp->response_body);

  sonic_free_request(req);
  sonic_free_response(resp);
}
