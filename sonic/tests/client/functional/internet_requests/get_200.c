#include "../../../../sonic.h"

#include <stdlib.h>

int main(int argc, char **argv) {
  sonic_response_t *resp = sonic_get("http://api.genderize.io/?name=john");

  if (resp->failed) {
    printf("request failed: %s \n", resp->error);

    exit(1);
  }

  printf("STATUS:%d|", resp->status);
  printf("BODY:%.*s", (int)resp->response_body_size, resp->response_body);

  fflush(stdout);

  free(resp->response_body);

  sonic_free_response(resp);
}
