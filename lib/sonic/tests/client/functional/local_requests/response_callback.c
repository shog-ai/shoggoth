#include "../../../../sonic.h"

#include <stdlib.h>

const char *downloaded_file_path = "./downloaded_file.bin";

void response_callback(char *data, u64 size, void *user_pointer) {
  FILE *file = fopen(downloaded_file_path, "a");
  if (file == NULL) {
    perror("Error opening file");
    return;
  }

  size_t bytes_written = fwrite(data, 1, size, file);
  if (bytes_written != size) {
    perror("Error writing to file");
  }

  fclose(file);
}

int main(int argc, char **argv) {
  FILE *file = fopen(downloaded_file_path, "w");
  if (file == NULL) {
    perror("Error opening file");
    return 1;
  }
  fclose(file);

  sonic_request_t *req =
      sonic_new_request(METHOD_GET, "http://127.0.0.1:5000/response_callback");

  sonic_request_set_response_callback(req, response_callback, NULL);

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
