#include "../../../../sonic.h"

#include <stdlib.h>

int main(int argc, char **argv) {
  sonic_request_t *req =
      sonic_new_request(METHOD_GET, "http://165.227.126.8:80/?name=john");
  sonic_add_header(req, "Host", "api.genderize.io");

  sonic_response_t *resp = sonic_send_request(req);

  if (resp->failed) {
    printf("request failed: %s \n", resp->error);

    exit(1);
  }

  printf("STATUS:%d|", resp->status);
  printf("BODY:%.*s", (int)resp->response_body_size, resp->response_body);

  fflush(stdout);

  free(resp->response_body);

  sonic_free_request(req);
  sonic_free_response(resp);
}
